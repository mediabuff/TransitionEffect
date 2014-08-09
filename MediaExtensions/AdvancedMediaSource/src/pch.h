//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved

#pragma once

// Windows Header Files:
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfapi.h>
#include <mferror.h>
#include <Mfreadwrite.h>
#include <d3d11_2.h>
#include <d2d1_2.h>
#include <DirectXMath.h>
#include <dwrite_2.h>
#include <wincodec.h>

#include <assert.h>

#include <tchar.h>
#include <Strsafe.h>
#include <objidl.h>
#include <new>

#include <wrl\client.h>
#include <wrl\implements.h>
#include <wrl\ftm.h>
#include <wrl\event.h>
#include <wrl\wrappers\corewrappers.h>
#include <windows.media.h>

#include <ppltasks.h>

#include "Common.h"

using namespace Platform;
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace Windows::Media::Core;
