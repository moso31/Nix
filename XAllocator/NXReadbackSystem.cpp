//
//D3D12_HEAP_PROPERTIES heapProperties = {};
//heapProperties.Type = D3D12_HEAP_TYPE_READBACK;
//m_pDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_pResource));
//m_pResource->Map(0, nullptr, reinterpret_cast<void**>(&m_pResourceData));