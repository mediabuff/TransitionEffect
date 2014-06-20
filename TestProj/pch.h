//
// pch.h
// Header for standard system include files.
//

#pragma once

#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfapi.h>
#include <mferror.h>
#include <Mfreadwrite.h>
#include <d3d11.h>
#include <D2d1helper.h>

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

using namespace Platform;
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;