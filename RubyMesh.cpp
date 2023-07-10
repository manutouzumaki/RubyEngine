#include "RubyMesh.h"

#include "JsonParser/JsonParser.h"

namespace Ruby
{

    struct Buffer {
        void* data;
        size_t size;
    };

    static Buffer ReadEntireFile(const char* filepath)
    {
        Buffer result{};

        HANDLE hFile = CreateFileA(filepath, GENERIC_READ,
            FILE_SHARE_READ, 0, OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL, 0);

        if (hFile == INVALID_HANDLE_VALUE) {
            printf("Error openging file: %s\n", filepath);
            return result;
        }

        LARGE_INTEGER bytesToRead = {};
        GetFileSizeEx(hFile, &bytesToRead);
        if (bytesToRead.QuadPart <= 0)
        {
            printf("Error file: %s is empty\n", filepath);
            return result;
        }

        char* fileData = new char[bytesToRead.QuadPart];
        size_t bytesReaded = 0;

        if (!ReadFile(hFile, (LPVOID)fileData, bytesToRead.QuadPart, (LPDWORD)&bytesReaded, 0))
        {
            printf("Error reading file: %s\n", filepath);
            return result;
        }

        result.data = (void*)fileData;
        result.size = bytesReaded;

        CloseHandle(hFile);

        return result;

    }

    Buffer GetAttributeAtIndex(JsonObject* root, UINT i, Buffer srcBuffer) {

        Buffer result{};

        JsonObject* accessors = root->GetChildByName("accessors");
        JsonValue* value = accessors->GetFirstValue();
        UINT j = 0;
        while (j++ < i) { value = value->next; }
        JsonObject* bufferView = value->valueObject;

        UINT32 bufferViewIndex = (UINT32)bufferView->GetFirstValue()->valueFloat;
        UINT32 componentType = (UINT32)bufferView->GetSiblingByName("componentType")->GetFirstValue()->valueFloat;
        UINT32 count = (UINT32)bufferView->GetSiblingByName("count")->GetFirstValue()->valueFloat;
        const char* type = bufferView->GetSiblingByName("type")->GetFirstValue()->valueChar;

        JsonObject* bufferViews = root->GetChildByName("bufferViews");

        value = bufferViews->GetFirstValue();
        j = 0;
        while (j++ < bufferViewIndex) value = value->next;
        JsonObject* buffer = value->valueObject;

        UINT32 bufferIndex = (UINT32)buffer->GetFirstValue()->valueFloat;
        UINT32 byteLength = (UINT32)buffer->GetSiblingByName("byteLength")->GetFirstValue()->valueFloat;
        UINT32 byteOffset = (UINT32)buffer->GetSiblingByName("byteOffset")->GetFirstValue()->valueFloat;
        UINT32 target = (UINT32)buffer->GetSiblingByName("target")->GetFirstValue()->valueFloat;

        result.data = (UINT8*)srcBuffer.data + byteOffset;
        result.size = (size_t)byteLength;

        return result;

    }

}

namespace Ruby
{

    MeshGeometry::MeshGeometry()
    {

    }

    MeshGeometry::~MeshGeometry()
    {
        if (mVB) mVB->Release();
        if (mIB) mIB->Release();
    }

    template<typename VertexType>
    void MeshGeometry::SetVertices(ID3D11Device* device, const VertexType* vertices, UINT count)
    {
        if (mVB) mVB->Release();

        D3D11_BUFFER_DESC vbd;
        vbd.Usage = D3D11_USAGE_IMMUTABLE;
        vbd.ByteWidth = sizeof(VertexType) * count;
        vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vbd.CPUAccessFlags = 0;
        vbd.MiscFlags = 0;
        vbd.StructureByteStride = 0;
        D3D11_SUBRESOURCE_DATA vinitData;
        vinitData.pSysMem = vertices;
        HRESULT result = device->CreateBuffer(&vbd, &vinitData, &mVB);
        if (FAILED(result))
        {
            MessageBox(0, "Error: failed loading vertex data", 0, 0);
        }

        mVertexStride = sizeof(VertexType);

    }

    void MeshGeometry::SetIndices(ID3D11Device* device, const USHORT* indices, UINT count)
    {
        if (mIB) mIB->Release();

        D3D11_BUFFER_DESC ibd;
        ibd.Usage = D3D11_USAGE_IMMUTABLE;
        ibd.ByteWidth = sizeof(USHORT) * count;
        ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
        ibd.CPUAccessFlags = 0;
        ibd.MiscFlags = 0;
        ibd.StructureByteStride = 0;
        D3D11_SUBRESOURCE_DATA iinitData;
        iinitData.pSysMem = indices;
        HRESULT result = device->CreateBuffer(&ibd, &iinitData, &mIB);
        if (FAILED(result))
        {
            MessageBox(0, "Error: falied loading index data", 0, 0);
        }
        mIndexBufferFormat = DXGI_FORMAT_R16_UINT;
        mIndicesCount = count;
    }

    void MeshGeometry::SetSubsetTable(std::vector<Subset>& subsetTable)
    {
        mSubsetTable = subsetTable;
    }

    std::vector<Ruby::MeshGeometry::Subset>& MeshGeometry::GetSubsetTable()
    {
        return mSubsetTable;
    }


    void MeshGeometry::Draw(ID3D11DeviceContext* dc, UINT subsetId)
    {
        UINT offet = 0;
        dc->IASetVertexBuffers(0, 1, &mVB, &mVertexStride, &offet);
        dc->IASetIndexBuffer(mIB, mIndexBufferFormat, 0);
        Subset subset = mSubsetTable[subsetId];
        dc->DrawIndexed(subset.IndexCount, subset.IndexStart, 0);
    }

    Mesh::Mesh(ID3D11Device* device,
               const std::string modelFilename,
               const std::string modelBinFilename,
               std::string textureFilepath)
        : ModelMesh()
    {

        // TODO: load gltf mesh
        JsonParser json = JsonParser();
        json.ParseFile(modelFilename.c_str());
        JsonObject* root = json.GetRoot();

        Ruby::Buffer bin = Ruby::ReadEntireFile(modelBinFilename.c_str());

        JsonObject* meshes = root->GetChildByName("meshes");

        JsonObject* primitives = meshes->GetFirstValue()->valueObject->GetSibling();

        JsonValue* value = primitives->GetFirstValue();

        int subsetId = 0;
        std::vector<MeshGeometry::Subset> subsetTable;

        while (value)
        {
            JsonObject* attributes = value->valueObject;

            UINT32 positionIndex = (UINT32)attributes->GetChildByName("POSITION")->GetFirstValue()->valueFloat;
            UINT32 texcoordIndex = (UINT32)attributes->GetChildByName("TEXCOORD_0")->GetFirstValue()->valueFloat;
            UINT32 normalIndex = (UINT32)attributes->GetChildByName("NORMAL")->GetFirstValue()->valueFloat;
            UINT32 tangentIndex = (UINT32)attributes->GetChildByName("TANGENT")->GetFirstValue()->valueFloat;
            UINT32 indicesIndex = (UINT32)attributes->GetSiblingByName("indices")->GetFirstValue()->valueFloat;
            UINT32 materialIndex = -1;
            if (attributes->GetSiblingByName("material"))
            {
                materialIndex = (UINT32)attributes->GetSiblingByName("material")->GetFirstValue()->valueFloat;
            }

            Buffer positionData = GetAttributeAtIndex(root, positionIndex, bin);
            Buffer texcoordData = GetAttributeAtIndex(root, texcoordIndex, bin);
            Buffer normalData = GetAttributeAtIndex(root, normalIndex, bin);
            Buffer tangentData = GetAttributeAtIndex(root, tangentIndex, bin);
            Buffer indicesData = GetAttributeAtIndex(root, indicesIndex, bin);

            std::vector<XMFLOAT3> positions;
            positions.assign((XMFLOAT3*)positionData.data, (XMFLOAT3*)((UINT8*)positionData.data + positionData.size));
            std::vector<XMFLOAT2> texcoords;
            texcoords.assign((XMFLOAT2*)texcoordData.data, (XMFLOAT2*)((UINT8*)texcoordData.data + texcoordData.size));
            std::vector<XMFLOAT3> normals;
            normals.assign((XMFLOAT3*)normalData.data, (XMFLOAT3*)((UINT8*)normalData.data + normalData.size));
            std::vector<XMFLOAT4> tangents;
            tangents.assign((XMFLOAT4*)tangentData.data, (XMFLOAT4*)((UINT8*)tangentData.data + tangentData.size));
            std::vector<USHORT> indices;
            indices.assign((USHORT*)indicesData.data, (USHORT*)((UINT8*)indicesData.data + indicesData.size));


            MeshGeometry::Subset subset;

            subset.Id = subsetId++;
            subset.IndexStart = Indices.size();
            subset.IndexCount = indices.size();

            subsetTable.push_back(subset);

            USHORT indexOffset = Vertices.size();
            for (UINT i = 0; i < indices.size(); ++i)
            {
                Indices.push_back(indices[i] + indexOffset);
            }

            for (UINT i = 0; i < positions.size(); ++i)
            {
                Ruby::Vertex vertex{};
                vertex.Position = positions[i];
                vertex.Normal = normals[i];
                vertex.TangentU = XMFLOAT3(tangents[i].x, tangents[i].y, tangents[i].z);
                vertex.TexC = texcoords[i];
                Vertices.push_back(vertex);
            }            
            value = value->next;
        }


        JsonObject* materials = root->GetChildByName("materials");
        if (materials) {
            value = materials->GetFirstValue();
            while (value)
            {
                JsonObject* pbrMetallicRoughness = value->valueObject->GetSiblingByName("pbrMetallicRoughness");
                JsonObject* baseColorFactor = pbrMetallicRoughness->GetChildByName("baseColorFactor");
                JsonObject* metallicFactor = pbrMetallicRoughness->GetChildByName("metallicFactor");
                JsonObject* roughnessFactor = pbrMetallicRoughness->GetChildByName("roughnessFactor");

                Pbr::Material material = Pbr::Material();

                float baseColorR = baseColorFactor->firstValue->valueFloat;
                float baseColorG = baseColorFactor->firstValue->next->valueFloat;
                float baseColorB = baseColorFactor->firstValue->next->next->valueFloat;
                float baseColorA = baseColorFactor->firstValue->next->next->next->valueFloat;

                float metallicFactorFloat = metallicFactor->firstValue->valueFloat;
                float roughnessFactorFloat = roughnessFactor->firstValue->valueFloat;

                material.Albedo = { baseColorR, baseColorG, baseColorB, baseColorA };
                material.Metallic = metallicFactorFloat;
                material.Roughness = roughnessFactorFloat;
                material.Ao = 1.0f;

                Mat.push_back(material);

                value = value->next;
            }
        }
        else {
            Pbr::Material material = Pbr::Material();

            material.Albedo = { 0.2f, 0.0f, 0.2f, 1.0f };
            material.Metallic = 0.0f;
            material.Roughness = 0.5f;
            material.Ao = 1.0f;

            Mat.push_back(material);
        }




        delete bin.data;

        ModelMesh.SetVertices<Vertex>(device, Vertices.data(), Vertices.size());
        ModelMesh.SetIndices(device, Indices.data(), Indices.size());
        ModelMesh.SetSubsetTable(subsetTable);
    }

    Mesh::~Mesh()
    {
    
    }

    float Line::Lenght()
    {
        XMVECTOR A = XMVectorSet(a.x, a.y, a.z, 1.0f);
        XMVECTOR B = XMVectorSet(b.x, b.y, b.z, 1.0f);
        XMVECTOR AB = B - A;
        XMVECTOR dot = XMVector3Dot(AB, AB);
        XMFLOAT3 dotFloat;
        XMStoreFloat3(&dotFloat, dot);
        return sqrtf(dotFloat.x);
    }

    float Line::LenghtSq()
    {
        XMVECTOR A = XMVectorSet(a.x, a.y, a.z, 1.0f);
        XMVECTOR B = XMVectorSet(b.x, b.y, b.z, 1.0f);
        XMVECTOR AB = B - A;
        XMVECTOR dot = XMVector3Dot(AB, AB);
        XMFLOAT3 dotFloat;
        XMStoreFloat3(&dotFloat, dot);
        return dotFloat.x;
    }

    int Line::IntersectPlane(Plane& plane, float& t)
    {
        XMVECTOR A = XMVectorSet(a.x, a.y, a.z, 1.0f);
        XMVECTOR B = XMVectorSet(b.x, b.y, b.z, 1.0f);
        XMVECTOR N = XMVectorSet(plane.n.x, plane.n.y, plane.n.z, 0.0f);
        XMVECTOR D = XMVectorSet(plane.d, plane.d, plane.d, 0.0f);
        XMVECTOR dot = (D - XMVector3Dot(N, A)) / XMVector3Dot(N, B - A);
        XMFLOAT3 dotFloat;
        XMStoreFloat3(&dotFloat, dot);

        t = dotFloat.x;

        if (t >= 0.0f && t <= 1.0f)
        {
            return 1;
        }
        return 0;
    }

    Vertex LerpVertex(Vertex a, Vertex b, float t)
    {
        XMVECTOR pos0 = XMVectorSet(a.Position.x, a.Position.y, a.Position.z, 1.0f);
        XMVECTOR nor0 = XMVectorSet(a.Normal.x, a.Normal.y, a.Normal.z, 0.0f);
        XMVECTOR tan0 = XMVectorSet(a.TangentU.x, a.TangentU.y, a.TangentU.z, 0.0f);
        XMVECTOR tex0 = XMVectorSet(a.TexC.x, a.TexC.y, 0.0f, 0.0f);

        XMVECTOR pos1 = XMVectorSet(b.Position.x, b.Position.y, b.Position.z, 1.0f);
        XMVECTOR nor1 = XMVectorSet(b.Normal.x, b.Normal.y, b.Normal.z, 0.0f);
        XMVECTOR tan1 = XMVectorSet(b.TangentU.x, b.TangentU.y, b.TangentU.z, 0.0f);
        XMVECTOR tex1 = XMVectorSet(b.TexC.x, b.TexC.y, 0.0f, 0.0f);

        XMVECTOR pos = (1.0f - t) * pos0 + t * pos1;
        XMVECTOR nor = XMVector3Normalize((1.0f - t) * nor0 + t * nor1);
        XMVECTOR tan = XMVector3Normalize((1.0f - t) * tan0 + t * tan1);
        XMVECTOR tex = (1.0f - t) * tex0 + t * tex1;

        Vertex vertex{};
        
        XMStoreFloat3(&vertex.Position, pos);
        XMStoreFloat3(&vertex.Normal, nor);
        XMStoreFloat3(&vertex.TangentU, tan);
        XMStoreFloat2(&vertex.TexC, tex);

        return vertex;
    }

    Mesh* Mesh::Clip(ID3D11Device* device, Plane& plane)
    {
        Mesh* result = new Mesh();
        result->Mat = Mat;
        result->Vertices = Vertices;

        std::vector<USHORT> indices;
        std::vector<MeshGeometry::Subset>& subsets = ModelMesh.GetSubsetTable();
        std::vector<MeshGeometry::Subset> newSubsets;

        XMVECTOR N = XMVector3Normalize(XMVectorSet(plane.n.x, plane.n.y, plane.n.z, 0.0f));
        XMStoreFloat3(&plane.n, N);

        for (int j = 0; j < subsets.size(); ++j)
        {
            MeshGeometry::Subset subset = subsets[j];
            UINT indexStart = indices.size();
            // for each triangle, try to clip it to the plane
            for (int i = subset.IndexStart; i < (subset.IndexStart + subset.IndexCount); i += 3)
            {
                // first need to see if the triangle intersect the plane
                // we thest the tree edges
                XMFLOAT3 vert0 = Vertices[Indices[i + 0]].Position;
                XMFLOAT3 vert1 = Vertices[Indices[i + 1]].Position;
                XMFLOAT3 vert2 = Vertices[Indices[i + 2]].Position;

                Line edges[3] = {
                    { vert0, vert1 },
                    { vert1, vert2 },
                    { vert2, vert0 }
                };
                Vertex triangleVertex[6] = {
                    Vertices[Indices[i + 0]], Vertices[Indices[i + 1]],
                    Vertices[Indices[i + 1]], Vertices[Indices[i + 2]],
                    Vertices[Indices[i + 2]], Vertices[Indices[i + 0]]
                };
                USHORT triangleIndices[6] = {
                    Indices[i + 0], Indices[i + 1],
                    Indices[i + 1], Indices[i + 2],
                    Indices[i + 2], Indices[i + 0]
                };
                
                UINT vertexIndex = 0;

                std::vector<USHORT> newIndices;

                for (int edgeIndex = 0; edgeIndex < 3; ++edgeIndex)
                {
                    Line edge = edges[edgeIndex];
                    
                    XMVECTOR A = XMVectorSet(edge.a.x, edge.a.y, edge.a.z, 1.0f);
                    XMVECTOR B = XMVectorSet(edge.b.x, edge.b.y, edge.b.z, 1.0f);

                    XMVECTOR O = N * plane.d;


                    XMVECTOR dotA = XMVector3Dot(A - O, N);
                    XMVECTOR dotB = XMVector3Dot(B - O, N);

                    XMFLOAT3 dotAFloat;
                    XMStoreFloat3(&dotAFloat, dotA);

                    XMFLOAT3 dotBFloat;
                    XMStoreFloat3(&dotBFloat, dotB);

                    Vertex vertexA = triangleVertex[vertexIndex + 0];
                    Vertex vertexB = triangleVertex[vertexIndex + 1];
                    USHORT indexA = triangleIndices[vertexIndex + 0];
                    USHORT indexB = triangleIndices[vertexIndex + 1];

                    // we dont want to recreate the vertices, we want to add new vertices and reac reate the indices
                    if (dotAFloat.x >= 0.0f && dotBFloat.x >= 0.0f)
                    {
                        // Here we know the edge not intersect the plane
                        newIndices.push_back(indexB);
                    }
                    else if (dotAFloat.x >= 0.0f && dotBFloat.x < 0.0f)
                    {
                        float t = 0.0f;
                        edge.IntersectPlane(plane, t);
                        // for now just interpolate the position
                        Vertex vertex = LerpVertex(vertexA, vertexB, t);
                        newIndices.push_back(result->Vertices.size());
                        result->Vertices.push_back(vertex);
                    }
                    else if (dotAFloat.x < 0.0f && dotBFloat.x < 0.0f)
                    {
                        // Here we know the edge not intersect the plane
                    }
                    else if (dotAFloat.x < 0.0f && dotBFloat.x >= 0.0f)
                    {
                        float t = 0.0f;
                        edge.IntersectPlane(plane, t);
                        Vertex vertex = LerpVertex(vertexA, vertexB, t);
                        newIndices.push_back(result->Vertices.size());
                        newIndices.push_back(indexB);
                        result->Vertices.push_back(vertex);
                    }

                    vertexIndex += 2;

                }

                // now we have the clip rectangle
                // but we need to triagulize this becouse its not a triangle any more
                if (newIndices.size() >= 4)
                {
                    indices.push_back(newIndices[0]);
                    indices.push_back(newIndices[1]);
                    indices.push_back(newIndices[2]);

                    indices.push_back(newIndices[0]);
                    indices.push_back(newIndices[2]);
                    indices.push_back(newIndices[3]);
                }
                else
                {
                    for (int index = 0; index < newIndices.size(); ++index)
                    {
                        indices.push_back(newIndices[index]);
                    }
                }
            }
            subset.IndexStart = indexStart;
            subset.IndexCount = indices.size() - subset.IndexStart;
            newSubsets.push_back(subset);
        }
        result->ModelMesh.SetVertices(device, result->Vertices.data(), result->Vertices.size());
        result->ModelMesh.SetIndices(device, indices.data(), indices.size());
        result->ModelMesh.SetSubsetTable(newSubsets);
        result->Indices = indices;
        return result;
    }
}
