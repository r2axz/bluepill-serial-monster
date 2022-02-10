/*
 * MIT License 
 * 
 * Copyright (c) 2022 Kirill Kotyagin
 */

#ifndef VERSION_H
#define VERSION_H

#ifndef DEVICE_VERSION_MAJOR
#define DEVICE_VERSION_MAJOR    0
#endif

#ifndef DEVICE_VERSION_MINOR
#define DEVICE_VERSION_MINOR    0
#endif

#ifndef DEVICE_VERSION_REVISION
#define DEVICE_VERSION_REVISION 0
#endif

#define __DEVICE_VERSION_STRING(major,minor,revision) "v" #major "." #minor "." #revision
#define _DEVICE_VERSION_STRING(major,minor,revision) __DEVICE_VERSION_STRING(major,minor,revision)
#define DEVICE_VERSION_STRING _DEVICE_VERSION_STRING(DEVICE_VERSION_MAJOR,DEVICE_VERSION_MINOR,DEVICE_VERSION_REVISION)

#endif // VERSION_H
