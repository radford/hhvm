/*
   +----------------------------------------------------------------------+
   | HipHop for PHP                                                       |
   +----------------------------------------------------------------------+
   | Copyright (c) 2010-2014 Facebook, Inc. (http://www.facebook.com)     |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
*/

#include "hphp/runtime/base/plain-stream-wrapper.h"
#include "hphp/runtime/base/file-repository.h"
#include "hphp/runtime/base/runtime-error.h"
#include "hphp/runtime/base/plain-file.h"
#include "hphp/runtime/base/directory.h"
#include "hphp/runtime/server/static-content-cache.h"
#include "hphp/system/constants.h"
#include "hphp/util/file-util.h"
#include <memory>

namespace HPHP {
///////////////////////////////////////////////////////////////////////////////

MemFile* PlainStreamWrapper::openFromCache(const String& filename,
                                           const String& mode) {
  if (!StaticContentCache::TheFileCache) {
    return nullptr;
  }

  String relative =
    FileCache::GetRelativePath(File::TranslatePath(filename).c_str());
  std::unique_ptr<MemFile> file(NEWOBJ(MemFile)());
  bool ret = file->open(relative, mode);
  if (ret) {
    return file.release();
  }
  return nullptr;
}

File* PlainStreamWrapper::open(const String& filename, const String& mode,
                              int options, CVarRef context) {
  if (MemFile *file = openFromCache(filename, mode)) {
    return file;
  }

  String fname;
  if (options & File::USE_INCLUDE_PATH) {
    struct stat s;
    fname = Eval::resolveVmInclude(filename.get(), "", &s);
  } else {
    fname = File::TranslatePath(fname);
  }

  std::unique_ptr<PlainFile> file(NEWOBJ(PlainFile)());
  bool ret = file->open(fname, mode);
  if (!ret) {
    raise_warning("%s", file->getLastError().c_str());
    return nullptr;
  }
  return file.release();
}

Directory* PlainStreamWrapper::opendir(const String& path) {
  std::unique_ptr<PlainDirectory> dir(
    NEWOBJ(PlainDirectory)(File::TranslatePath(path))
  );
  if (!dir->isValid()) {
    raise_warning("%s", dir->getLastError().c_str());
    return nullptr;
  }
  return dir.release();
}

int PlainStreamWrapper::access(const String& path, int mode) {
  return ::access(File::TranslatePath(path).data(), mode);
}

int PlainStreamWrapper::stat(const String& path, struct stat* buf) {
  return ::stat(File::TranslatePath(path).data(), buf);
}

int PlainStreamWrapper::lstat(const String& path, struct stat* buf) {
  return ::lstat(File::TranslatePath(path).data(), buf);
}

int PlainStreamWrapper::unlink(const String& path) {
  return ::unlink(File::TranslatePath(path).data());
}

int PlainStreamWrapper::rmdir(const String& path, int options) {
  return ::rmdir(File::TranslatePath(path).data());
}

int PlainStreamWrapper::rename(const String& oldname, const String& newname) {
  int ret =
    RuntimeOption::UseDirectCopy ?
      FileUtil::directRename(File::TranslatePath(oldname).data(),
                             File::TranslatePath(newname).data())
                                 :
      FileUtil::rename(File::TranslatePath(oldname).data(),
                       File::TranslatePath(newname).data());
  return ret;
}

int PlainStreamWrapper::mkdir(const String& path, int mode, int options) {
  if (options & k_STREAM_MKDIR_RECURSIVE)
    return FileUtil::mkdir(File::TranslatePath(path).data(), mode);
  return ::mkdir(File::TranslatePath(path).data(), mode);
}

///////////////////////////////////////////////////////////////////////////////
}
