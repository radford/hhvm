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

#include "hphp/runtime/base/file-stream-wrapper.h"
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

static bool valid(const String& filename) {
  if (filename.data()[7] == '/') { // not just file://, but file:///
    return true;
  }
  raise_warning("Only hostless file::// URLs are supported: %s", filename.data());
  return false;
}

static String strip(const String& filename) {
  assert(valid(filename));
  return filename.substr(sizeof("file://") - 1);
}

File* FileStreamWrapper::open(const String& filename, const String& mode,
                              int options, CVarRef context) {
  if (!valid(filename)) return nullptr;
  std::unique_ptr<PlainFile> file(NEWOBJ(PlainFile)());
  bool ret = file->open(strip(filename), mode);
  if (!ret) {
    raise_warning("%s", file->getLastError().c_str());
    return nullptr;
  }
  return file.release();
}

Directory* FileStreamWrapper::opendir(const String& path) {
  if (!valid(path)) return nullptr;
  std::unique_ptr<PlainDirectory> dir(
    NEWOBJ(PlainDirectory)(strip(path))
  );
  if (!dir->isValid()) {
    raise_warning("%s", dir->getLastError().c_str());
    return nullptr;
  }
  return dir.release();
}

int FileStreamWrapper::stat(const String& path, struct stat* buf) {
  return valid(path) ? ::stat(strip(path).data(), buf) : -1;
}

int FileStreamWrapper::lstat(const String& path, struct stat* buf) {
  return valid(path) ? ::lstat(strip(path).data(), buf) : -1;
}

int FileStreamWrapper::unlink(const String& path) {
  return valid(path) ? ::unlink(strip(path).data()) : -1;
}

int FileStreamWrapper::rmdir(const String& path, int options) {
  return valid(path) ? ::rmdir(strip(path).data()) : -1;
}

int FileStreamWrapper::rename(const String& oldname, const String& newname) {
  return !valid(oldname) || !valid(newname) ? -1 :
    RuntimeOption::UseDirectCopy ?
      FileUtil::directRename(strip(oldname).data(),
                             strip(newname).data())
                                 :
      FileUtil::rename(strip(oldname).data(),
                       strip(newname).data());
}

int FileStreamWrapper::mkdir(const String& path, int mode, int options) {
  if (options & k_STREAM_MKDIR_RECURSIVE)
     return valid(path) && FileUtil::mkdir(strip(path).data(), mode) ? 0 : -1;
  return valid(path) ? ::mkdir(strip(path).data(), mode) : -1;
}

///////////////////////////////////////////////////////////////////////////////
}
