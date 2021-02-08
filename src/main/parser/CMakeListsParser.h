/* KDevelop CMake Support
 *
 * Copyright 2006 Matt Rogers <mattr@kde.org>
 * Copyright 2008 Aleix Pol <aleixpol@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 */

#pragma once

#include "CMakeFileContent.h"

namespace cmle { namespace parser {

CMakeFileContent readCMakeFile(const QByteArray& fileContent, bool* error);

}} // namespace cmle::parser
