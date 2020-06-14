#pragma once

typedef HRESULT(STDMETHODCALLTYPE* CreateShaderResourceView_T)(
	ID3D11Device* pDevice,
	_In_  ID3D11Resource* pResource,
	_In_opt_  const D3D11_SHADER_RESOURCE_VIEW_DESC* pDesc,
	_COM_Outptr_opt_  ID3D11ShaderResourceView** ppSRView);


namespace D3D11
{
	extern int32_t MaxFrameLatency;

	void InstallHooksIfLoaded();
}