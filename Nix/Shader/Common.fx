cbuffer ConstantBufferObject : register(b0)
{
	matrix m_world;
	matrix m_worldInverseTranspose;
	matrix m_view;
	matrix m_viewInverse;
	matrix m_viewTranspose;
	matrix m_viewInverseTranspose;
	matrix m_worldView;
	matrix m_worldViewInverseTranspose;
	matrix m_projection;
}