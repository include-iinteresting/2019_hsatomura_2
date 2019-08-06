#ifndef __DEVICE_DEPENDENT_OBJECTS_H__
#define __DEVICE_DEPENDENT_OBJECTS_H__
/**
*  @brief  デバイス依存オブジェクト(デバイスロストで破壊されるオブジェクト)のスーパークラス
*  @date   11.May.2019
*/

struct ID3D11DeviceContext;

class IDeviceDependentObject
{
public:
	virtual ~IDeviceDependentObject() = 0;
	virtual HRESULT RestoreDeviceObjects(ID3D11DeviceContext *pContext) = 0;
	virtual HRESULT ReleaseDeviceObjects() = 0;
};

#endif // !__DEVICE_DEPENDENT_OBJECTS_H__
