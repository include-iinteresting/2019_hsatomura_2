#ifndef __DEVICE_DEPENDENT_OBJECTS_H__
#define __DEVICE_DEPENDENT_OBJECTS_H__
/**
*  @brief  �f�o�C�X�ˑ��I�u�W�F�N�g(�f�o�C�X���X�g�Ŕj�󂳂��I�u�W�F�N�g)�̃X�[�p�[�N���X
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
