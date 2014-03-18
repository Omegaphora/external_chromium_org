# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import sys

import schema_util
from docs_server_utils import ToUnicode
from file_system import FileNotFoundError
from future import Gettable, Future
from path_util import AssertIsDirectory, AssertIsFile
from third_party.handlebar import Handlebar
from third_party.json_schema_compiler import json_parse
from third_party.json_schema_compiler.memoize import memoize


_SINGLE_FILE_FUNCTIONS = set()


def SingleFile(fn):
  '''A decorator which can be optionally applied to the compilation function
  passed to CompiledFileSystem.Create, indicating that the function only
  needs access to the file which is given in the function's callback. When
  this is the case some optimisations can be done.

  Note that this decorator must be listed first in any list of decorators to
  have any effect.
  '''
  _SINGLE_FILE_FUNCTIONS.add(fn)
  return fn


def Unicode(fn):
  '''A decorator which can be optionally applied to the compilation function
  passed to CompiledFileSystem.Create, indicating that the function processes
  the file's data as Unicode text.
  '''

  # The arguments passed to fn can be (self, path, data) or (path, data). In
  # either case the last argument is |data|, which should be converted to
  # Unicode.
  def convert_args(args):
    args = list(args)
    args[-1] = ToUnicode(args[-1])
    return args

  return lambda *args: fn(*convert_args(args))


class _CacheEntry(object):
  def __init__(self, cache_data, version):

    self._cache_data = cache_data
    self.version = version


class CompiledFileSystem(object):
  '''This class caches FileSystem data that has been processed.
  '''

  class Factory(object):
    '''A class to build a CompiledFileSystem backed by |file_system|.
    '''

    def __init__(self, object_store_creator):
      self._object_store_creator = object_store_creator

    def Create(self, file_system, compilation_function, cls, category=None):
      '''Creates a CompiledFileSystem view over |file_system| that populates
      its cache by calling |compilation_function| with (path, data), where
      |data| is the data that was fetched from |path| in |file_system|.

      The namespace for the compiled file system is derived similar to
      ObjectStoreCreator: from |cls| along with an optional |category|.
      '''
      assert isinstance(cls, type)
      assert not cls.__name__[0].islower()  # guard against non-class types
      full_name = [cls.__name__, file_system.GetIdentity()]
      if category is not None:
        full_name.append(category)
      def create_object_store(my_category):
        # The read caches can start populated (start_empty=False) because file
        # updates are picked up by the stat - but only if the compilation
        # function is affected by a single file. If the compilation function is
        # affected by other files (e.g. compiling a list of APIs available to
        # extensions may be affected by both a features file and the list of
        # files in the API directory) then this optimisation won't work.
        return self._object_store_creator.Create(
            CompiledFileSystem,
            category='/'.join(full_name + [my_category]),
            start_empty=compilation_function not in _SINGLE_FILE_FUNCTIONS)
      return CompiledFileSystem(file_system,
                                compilation_function,
                                create_object_store('file'),
                                create_object_store('list'))

    @memoize
    def ForJson(self, file_system):
      '''A CompiledFileSystem specifically for parsing JSON configuration data.
      These are memoized over file systems tied to different branches.
      '''
      return self.Create(file_system,
                         SingleFile(lambda _, data:
                             json_parse.Parse(ToUnicode(data))),
                         CompiledFileSystem,
                         category='json')

    @memoize
    def ForApiSchema(self, file_system):
      '''Creates a CompiledFileSystem for parsing raw JSON or IDL API schema
      data and formatting it so that it can be used by other classes, such
      as Model and APISchemaGraph.
      '''
      return self.Create(file_system,
                         SingleFile(Unicode(schema_util.ProcessSchema)),
                         CompiledFileSystem,
                         category='api-schema')

    @memoize
    def ForTemplates(self, file_system):
      '''Creates a CompiledFileSystem for parsing templates.
      '''
      return self.Create(
          file_system,
          SingleFile(lambda path, text: Handlebar(ToUnicode(text), name=path)),
          CompiledFileSystem)

    @memoize
    def ForUnicode(self, file_system):
      '''Creates a CompiledFileSystem for Unicode text processing.
      '''
      return self.Create(
        file_system,
        SingleFile(lambda _, text: ToUnicode(text)),
        CompiledFileSystem,
        category='text')

  def __init__(self,
               file_system,
               compilation_function,
               file_object_store,
               list_object_store):
    self._file_system = file_system
    self._compilation_function = compilation_function
    self._file_object_store = file_object_store
    self._list_object_store = list_object_store

  def _RecursiveList(self, path):
    '''Returns a Future containing the recursive directory listing of |path| as
    a flat list of paths.
    '''
    def split_dirs_from_files(paths):
      '''Returns a tuple (dirs, files) where |dirs| contains the directory
      names in |paths| and |files| contains the files.
      '''
      result = [], []
      for path in paths:
        result[0 if path.endswith('/') else 1].append(path)
      return result

    def add_prefix(prefix, paths):
      return [prefix + path for path in paths]

    # Read in the initial list of files. Do this eagerly (i.e. not part of the
    # asynchronous Future contract) because there's a greater chance to
    # parallelise fetching with the second layer (can fetch multiple paths).
    try:
      first_layer_dirs, first_layer_files = split_dirs_from_files(
          self._file_system.ReadSingle(path).Get())
    except FileNotFoundError:
      return Future(exc_info=sys.exc_info())

    if not first_layer_dirs:
      return Future(value=first_layer_files)

    second_layer_listing = self._file_system.Read(
        add_prefix(path, first_layer_dirs))

    def resolve():
      def get_from_future_listing(futures):
        '''Recursively lists files from directory listing |futures|.
        '''
        dirs, files = [], []
        for dir_name, listing in futures.Get().iteritems():
          new_dirs, new_files = split_dirs_from_files(listing)
          # |dirs| are paths for reading. Add the full prefix relative to
          # |path| so that |file_system| can find the files.
          dirs += add_prefix(dir_name, new_dirs)
          # |files| are not for reading, they are for returning to the caller.
          # This entire function set (i.e. GetFromFileListing) is defined to
          # not include the fetched-path in the result, however, |dir_name|
          # will be prefixed with |path|. Strip it.
          assert dir_name.startswith(path)
          files += add_prefix(dir_name[len(path):], new_files)
        if dirs:
          files += get_from_future_listing(self._file_system.Read(dirs))
        return files

      return first_layer_files + get_from_future_listing(second_layer_listing)

    return Future(delegate=Gettable(resolve))

  def GetFromFile(self, path):
    '''Calls |compilation_function| on the contents of the file at |path|.  If
    |binary| is True then the file will be read as binary - but this will only
    apply for the first time the file is fetched; if already cached, |binary|
    will be ignored.
    '''
    AssertIsFile(path)

    try:
      version = self._file_system.Stat(path).version
    except FileNotFoundError:
      return Future(exc_info=sys.exc_info())

    cache_entry = self._file_object_store.Get(path).Get()
    if (cache_entry is not None) and (version == cache_entry.version):
      return Future(value=cache_entry._cache_data)

    future_files = self._file_system.ReadSingle(path)
    def resolve():
      cache_data = self._compilation_function(path, future_files.Get())
      self._file_object_store.Set(path, _CacheEntry(cache_data, version))
      return cache_data
    return Future(delegate=Gettable(resolve))

  def GetFromFileListing(self, path):
    '''Calls |compilation_function| on the listing of the files at |path|.
    Assumes that the path given is to a directory.
    '''
    AssertIsDirectory(path)

    try:
      version = self._file_system.Stat(path).version
    except FileNotFoundError:
      return Future(exc_info=sys.exc_info())

    cache_entry = self._list_object_store.Get(path).Get()
    if (cache_entry is not None) and (version == cache_entry.version):
      return Future(value=cache_entry._cache_data)

    recursive_list_future = self._RecursiveList(path)
    def resolve():
      cache_data = self._compilation_function(path, recursive_list_future.Get())
      self._list_object_store.Set(path, _CacheEntry(cache_data, version))
      return cache_data
    return Future(delegate=Gettable(resolve))

  def GetFileVersion(self, path):
    cache_entry = self._file_object_store.Get(path).Get()
    if cache_entry is not None:
      return cache_entry.version
    return self._file_system.Stat(path).version

  def GetFileListingVersion(self, path):
    if not path.endswith('/'):
      path += '/'
    cache_entry = self._list_object_store.Get(path).Get()
    if cache_entry is not None:
      return cache_entry.version
    return self._file_system.Stat(path).version

  def FileExists(self, path):
    return self._file_system.Exists(path)
