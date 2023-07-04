#pragma once

#include <d3d11.h>
#include <d3dx11.h>
#include <d3dx11Effect.h>
#include <DirectXMath.h>

#include <vector>
#include <string>

#include "LightHelper.h"
#include "GeometryGenerator.h"

namespace Ruby
{
    class MeshGeometry
    {
    public:
        struct Subset
        {
            Subset()
                : Id(-1),
                VertexStart(0), VertexCount(0),
                IndexStart(0), IndexCount(0) {}

            UINT Id;
            UINT VertexStart;
            UINT VertexCount;
            UINT IndexStart;
            UINT IndexCount;
        };
    public:
        MeshGeometry();
        ~MeshGeometry();
        template<typename VertexType>
        void SetVertices(ID3D11Device* device, const VertexType* vertices, UINT count);
        void SetIndices(ID3D11Device* device, const USHORT* indices, UINT count);
        void SetSubsetTable(std::vector<Subset>& subsetTable);
        void Draw(ID3D11DeviceContext* dc, UINT subsetId);
    private:
        MeshGeometry(const MeshGeometry& rhs);
        MeshGeometry& operator=(const MeshGeometry& rhs);
    private:
        ID3D11Buffer* mVB;
        ID3D11Buffer* mIB;
        DXGI_FORMAT mIndexBufferFormat;
        UINT mVertexStride;
        std::vector<Subset> mSubsetTable;

        UINT mIndicesCount;

    };

    class Mesh
    {
    public:
        Mesh() {};
        Mesh(ID3D11Device* device,
            const std::string modelFilename,
            const std::string modelBinFilename,
            std::string textureFilepath);
        ~Mesh();

        UINT SubsetCount;
        //std::vector<Material> Mat;
        std::vector<Pbr::Material> Mat;
        std::vector<ID3D11ShaderResourceView*> DiffuseMapSRV;
        std::vector<ID3D11ShaderResourceView*> NormalMapSRV;

        std::vector<Vertex> Vertices;
        std::vector<USHORT> Indices;
        std::vector<MeshGeometry::Subset> Subsets;

        MeshGeometry ModelMesh;
    };

}



