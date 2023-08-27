#pragma once

#define SafeDeleteArray(x) { delete[] x; x = nullptr; }
#define SafeDelete(x) { delete x; x = nullptr; }
#define SafeReleaseCOM(x) { if (x) { x->Release(); x = nullptr; } }
#define SafeRelease(x) { if (x) { x->Release(); SafeDelete(x); } }
