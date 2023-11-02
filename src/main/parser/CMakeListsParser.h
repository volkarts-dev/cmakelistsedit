// KDevelop CMake Support
// Copyright 2006 Matt Rogers <mattr@kde.org>
// Copyright 2008 Aleix Pol <aleixpol@gmail.com>
// Copyright 2021-2023, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "CMakeFileContent.h"

namespace cmle { namespace parser {

CMakeFileContent readCMakeFile(const QByteArray& fileContent, bool* error);

}} // namespace cmle::parser
