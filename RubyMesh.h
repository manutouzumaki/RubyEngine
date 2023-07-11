#pragma once

#include <d3d11.h>
#include <d3dx11.h>
#include <DirectXMath.h>

#include <vector>
#include <string>

#include "LightHelper.h"
#include "GeometryGenerator.h"

namespace Ruby
{
    struct Plane
    {
        XMFLOAT3 n;
        float d; // dot(n, a): a is a point on the plane
    };

    struct Line
    {
        XMFLOAT3 a;
        XMFLOAT3 b;

        int IntersectPlane(Plane& plane, float& t);
        float Lenght();
        float LenghtSq();
    };


    class MeshGeometry
    {
    public:
        struct Subset
        {
            Subset()
                : Id(-1),
                IndexStart(0), IndexCount(0) {}

            UINT Id;
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
        std::vector<Subset>& GetSubsetTable();
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

        Mesh* Clip(ID3D11Device* device, Plane& plane);
        void GetBoundingBox(XMFLOAT3& min, XMFLOAT3& max);

        std::vector<Pbr::Material> Mat;

        std::vector<Vertex> Vertices;
        std::vector<USHORT> Indices;
        std::vector<MeshGeometry::Subset> Subsets;

        MeshGeometry ModelMesh;
    };
}



