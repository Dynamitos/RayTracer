const static float PI = 3.1415926535897932f;

struct Camera
{
    float3 cameraPosition;
    float f;
    float3 cameraForward;
    float S_O;
    float3 fogEmm;
    float ks;
    float A;
    float ka;
};

struct MaterialParameter
{
    float3 albedo = float3(1, 1, 1);
    float alpha = 1;
    float3 specularColor = float3(1, 1, 1);
    float shininess = 0.04;
    float3 emissive = float3(0, 0, 0);
    float3 shade(float3 normal, float3 viewDir, float3 lightDir, float3 lightColor)
    {
        float diffuse = max(dot(normal, lightDir), 0);
        float3 h = normalize(lightDir + viewDir);
        float specular = pow(clamp(dot(normal, h), 0, 1), shininess);

        return (albedo * diffuse * lightColor);
    }
};

struct ModelReference
{
  uint32_t positionOffset = 0;
  uint32_t indicesOffset = 0;
  uint32_t numIndices = 0;
};

struct PointLight
{
  float3 position = float3(0, 0, 0);
  float3 color = float3(1, 1, 1);
  float attenuation = 1;
};

struct DirectionalLight
{
  float3 direction = float3(0, 1, 0);
  float3 color = float3(1, 1, 1);
};

struct RaytracingParams
{
    Camera cam;
    RaytracingAccelerationStructure scene;
    RWTexture2D<float4> radianceAccumulator;
    RWTexture2D<float4> image;
    StructuredBuffer<ModelReference> modelData;
    StructuredBuffer<MaterialParameter> materialData;
    StructuredBuffer<float> positions;
    StructuredBuffer<float> texCoords;
    StructuredBuffer<float> normals;
    StructuredBuffer<DirectionalLight> directionalLights;
    StructuredBuffer<PointLight> pointLights;
    StructuredBuffer<uint32_t> indexBuffer;
};
ParameterBlock<RaytracingParams> pParams;

struct Vertex
{
    float3 position;
    float2 texCoords;
    float3 normal;
    static Vertex interpolate(Vertex f0, Vertex f1, Vertex f2, float3 barycentricCoords)
    {
        Vertex vert;
        vert.position = f0.position * barycentricCoords.x + f1.position * barycentricCoords.y + f2.position * barycentricCoords.z;
        vert.texCoords = f0.texCoords * barycentricCoords.x + f1.texCoords * barycentricCoords.y + f2.texCoords * barycentricCoords.z;
        vert.normal = f0.normal * barycentricCoords.x + f1.normal * barycentricCoords.y + f2.normal * barycentricCoords.z;
        return vert;
    }
};

Vertex loadVertex(uint32_t vertexIndex)
{
    Vertex vert;
    vert.position = float3(pParams.positions[vertexIndex * 3 + 0], pParams.positions[vertexIndex * 3 + 1], pParams.positions[vertexIndex * 3 + 2]);
    vert.texCoords = float2(pParams.texCoords[vertexIndex * 2 + 0], pParams.texCoords[vertexIndex * 2 + 1]);
    vert.normal = float3(pParams.normals[vertexIndex * 3 + 0], pParams.normals[vertexIndex * 3 + 1], pParams.normals[vertexIndex * 3 + 2]);
    return vert;
}

struct SampleParams
{
    uint pass;
    uint samplesPerPixel;
    uint numDirectionalLights;
    uint numPointLights;
};
layout(push_constant)
ConstantBuffer<SampleParams> pSamps;

struct Ray
{
    float3 o;
    float3 d;
};

struct RayPayload
{
    float3 light;
    float emissive;
    uint depth;
    bool hit;
    bool anyHit;
};

float3 rand01(uint3 x){                   // pseudo-random number generator
    for (int i=3; i-->0;) x = ((x>>8U)^x.yzx)*1103515245U;
    return float3(x)*(1.0/float(0xffffffffU));
}