#include <metal_stdlib>
#include <simd/simd.h>
#include <metal_numeric>

using namespace metal;

using namespace raytracing;


enum class MaterialType
{
  BlinnPhong
};

struct GPUCamera
{
    packed_float3 position;
    float f;
    packed_float3 forward;
    float S_O;
    packed_float3 fogEmm;
    float ks;
    float A;
    float ka;
    float2 sensorSize;
    uint width;
    uint height;
};

struct SampleParams
{
    uint pass;
    uint samplesPerPixel;
    uint numDirectionalLights;
    uint numPointLights;
};

struct Payload
{
  float3 rnd01;
  float3 accumulatedRadiance = float3(0);
  float3 accumulatedMaterial = float3(1);
  uint depth = 0;
  float emissive = 1;
};

struct ModelReference
{
  uint positionOffset = 0;
  uint numPositions = 0;
  uint indicesOffset = 0;
  uint numIndices = 0;
};

struct HitInfo
{
  float t = numeric_limits<float>::max();
  float3 position;
  float3 normal;
  // not entirely sure what that does
  // its the normal being flipped based on some dot product
  float3 normalLight;
  float2 texCoords;
};

struct BRDF
{
  float3 albedo = float3(1, 1, 1);
  float alpha = 1;
  float3 specularColor = float3(1, 1, 1);
  float shininess = 0.004;
  float3 emissive = float3(0, 0, 0);
  MaterialType materialType;
  float3 evaluate(HitInfo hit, float3 viewDir, float3 lightDir, float3 lightColor)
  {
    float3 normal = hit.normal;
    float diffuse = max(dot(normal, lightDir), 0.0f);
    float3 h = normalize(lightDir + viewDir);
    float specular = pow(clamp(dot(normal, h), 0.0f, 1.0f), shininess);
    
    return (albedo * diffuse * lightColor) + float3(0.03, 0.03, 0.03);
  }
};


struct PointLight
{
  float3 position = float3(0, 0, 0);
  packed_float3 color = float3(1, 1, 1);
  float attenuation = 1;
};

struct DirectionalLight
{
  float3 direction = float3(0, 1, 0);
  float3 color = float3(1, 1, 1);
  };

float3 rand01(uint3 x)
{ // pseudo-random number generator
  for (int i = 3; i-- > 0;)
    x = ((x >> 8U) ^ uint3(x.y, x.z, x.x)) * 1103515245U;
  return float3(x) * (1.0f / float(0xffffffffU));
}

template<typename T, typename IndexType>
inline T interpolateVertexAttribute(constant T *attributes,
                                    uint offset,
                                    IndexType i0,
                                    IndexType i1,
                                    IndexType i2,
                                    float2 uv) {
    // Look up value for each vertex.
    const T T0 = attributes[offset + i0];
    const T T1 = attributes[offset + i1];
    const T T2 = attributes[offset + i2];

    // Compute the sum of the vertex attributes weighted by the barycentric coordinates.
    // The barycentric coordinates sum to one.
    return (1.0f - uv.x - uv.y) * T2 + uv.x * T0 + uv.y * T1;
}

kernel void computeKernel(
    uint2 threadId [[thread_position_in_grid]],
    constant GPUCamera& camera,
    constant SampleParams& sample,
    constant packed_uint3* indexBuffer [[buffer(0)]],
    constant packed_float3* positions [[buffer(1)]],
    constant packed_float2* texCoords [[buffer(2)]],
    constant packed_float3* normals [[buffer(3)]],
    constant ModelReference* modelRefs [[buffer(4)]],
    constant BRDF* materials [[buffer(5)]],
    constant DirectionalLight* directionalLights [[buffer(6)]],
    constant PointLight* pointLights [[buffer(7)]],
    constant MTLAccelerationStructureInstanceDescriptor* instances [[buffer(8)]],
    instance_acceleration_structure accelerationStructure [[buffer(9)]],
    texture2d<float, access::read_write> accumulator [[texture(0)]],
    texture2d<float, access::read_write> image [[texture(1)]]
)
{
    Payload payload;
    ray cam;
    cam.origin = camera.position;
    cam.direction = normalize(camera.forward);
    cam.max_distance = INFINITY;
    float3 cx =
    normalize(cross(cam.direction, abs(cam.direction.y) < 0.9 ? float3(0, 1, 0) : float3(0, 0, 1))),
    cy = cross(cx, cam.direction);
    const float2 sdim = camera.sensorSize; // sensor size (36 x 24 mm)
    
    float S_I = (camera.S_O * camera.f) / (camera.S_O - camera.f);
    
    //-- sample sensor
    uint2 pix = threadId;
    if(pix.x >= camera.width || pix.y >= camera.height)
        return;
    
    payload.rnd01 = rand01(uint3(pix, sample.pass));
    float2 rnd2 = 2.0f * float2(payload.rnd01.xy); // vvv tent filter sample
    float2 tent =
    float2(rnd2.x < 1 ? sqrt(rnd2.x) - 1 : 1 - sqrt(2 - rnd2.x), rnd2.y < 1 ? sqrt(rnd2.y) - 1 : 1 - sqrt(2 - rnd2.y));
    float2 s =
    ((float2(pix) + 0.5f * (0.5f + float2((sample.pass / 2) % 2, sample.pass % 2) + tent)) / float2(camera.width, camera.height) -
     0.5f) *
    sdim;
    float3 spos = cam.origin + cx * s.x + cy * s.y, lc = cam.origin + cam.direction * 0.035f; // sample on 3d sensor plane
    cam.origin = lc;
    cam.direction = normalize(lc - spos);                                                     // construct ray
    
    //-- setup lens
    float3 lensP = lc;
    float3 lensN = -cam.direction;
    float3 lensX = cross(lensN, float3(0, 1, 0)); // the exact vector doesnt matter
    float3 lensY = cross(lensN, lensX);
    
    float3 lensSample = lensP + payload.rnd01.x * camera.A * lensX + payload.rnd01.y * camera.A * lensY;
    
    float3 focalPoint = cam.origin + (camera.S_O + S_I) * cam.direction;
    float t = dot(focalPoint - cam.origin, lensN) / dot(cam.direction, lensN);
    float3 focus = cam.origin + t * cam.direction;
    cam.origin = lensSample;
    cam.direction = normalize(focus - lensSample); // TODO: Fix lens
    
    intersector<triangle_data, instancing> i;
    i.assume_geometry_type(geometry_type::triangle);
    i.force_opacity(forced_opacity::opaque);
    
    typename intersector<triangle_data, instancing>::result_type intersection;
    while(payload.depth < 12) {
        i.accept_any_intersection(false);
        
        intersection = i.intersect(cam, accelerationStructure, 0xff);
        
        if(intersection.type == intersection_type::none)
            break;
        
        uint instanceId = intersection.instance_id;
        constant ModelReference& ref = modelRefs[instanceId];
        
        HitInfo info;
        info.t = intersection.distance;
        const auto indices = indexBuffer[ref.indicesOffset + intersection.primitive_id];
        info.position = interpolateVertexAttribute(positions, ref.positionOffset, indices.x, indices.y, indices.z, intersection.triangle_barycentric_coord);
        info.texCoords = interpolateVertexAttribute(texCoords, ref.positionOffset, indices.x, indices.y, indices.z, intersection.triangle_barycentric_coord);
        info.normal = normalize(interpolateVertexAttribute(normals, ref.positionOffset, indices.x, indices.y, indices.z, intersection.triangle_barycentric_coord));
        info.normalLight = dot(info.normal, cam.direction) < 0 ? info.normal : -info.normal;
        
        BRDF brdf;
      brdf.albedo = float3(0, 1, 0);
        
        float p = max(max(brdf.albedo.x, brdf.albedo.y), brdf.albedo.z);
        if (payload.depth > 5)
        {
            if (payload.rnd01.z >= p)
                break;
            else
                payload.accumulatedMaterial /= p;
        }
        // emissive
        payload.accumulatedRadiance += payload.accumulatedMaterial * brdf.emissive * payload.emissive;
        payload.accumulatedMaterial *= brdf.albedo;
        
        // direct lighting
        for (uint l = 0; l < sample.numDirectionalLights; ++l)
        {
            // if there is an intersection, the light is occluded so no lighting
            ray shadowRay;
            shadowRay.origin = info.position + info.normal * 1e-3f;
            shadowRay.direction = -directionalLights[l].direction;
            shadowRay.max_distance = INFINITY;
            i.accept_any_intersection(true);
            intersection = i.intersect(shadowRay, accelerationStructure, 0xff);
            if(intersection.type == intersection_type::none)
            {
                payload.accumulatedRadiance += brdf.evaluate(info, -cam.direction, normalize(shadowRay.direction), directionalLights[l].color);
            }
        }
        for (uint l = 0; l < sample.numPointLights; ++l)
        {
            float3 lightDir = pointLights[l].position - info.position;
            ray shadowRay;
            shadowRay.origin = info.position + info.normal * 1e-3f;
            shadowRay.direction = lightDir;
            shadowRay.max_distance = 1;
            i.accept_any_intersection(true);
            intersection = i.intersect(shadowRay, accelerationStructure, 0xff);
            if (intersection.type == intersection_type::none)
            {
                float d = length(lightDir);
                float illuminance = max(1 - d / pointLights[l].attenuation, 0.0f);
                
                payload.accumulatedRadiance += illuminance * brdf.evaluate(info, -cam.direction, normalize(lightDir), pointLights[l].color);
            }
        }
        
        // TODO: Next Event Estimation for mesh lights
        
        // indirect lighting
        float r1 = 2 * M_PI_F * payload.rnd01.x;
        float r2 = payload.rnd01.y;
        float r2s = sqrt(r2);
        float3 w = info.normalLight;
        float3 u = normalize(cross(abs(w.x) > 0.1 ? float3(0, 1, 0) : float3(1, 0, 0), w));
        float3 v = cross(w, u);
        cam.origin = info.position;
        cam.direction = normalize(u * cos(r1) * r2s + v * sin(r1) * r2s + w * sqrt(1 - r2));
        payload.emissive = 0;
        payload.depth++;
    }
    float resolver = float(sample.samplesPerPixel) / float(sample.pass+1);
    float4 previous = float4(0);
    if(sample.pass != 0)
    {
      previous = accumulator.read(threadId);
    }
    float4 result = previous + float4(payload.accumulatedRadiance / float(sample.samplesPerPixel), 0);
    accumulator.write(result, threadId);
    image.write(pow(max(result * resolver, 0), float4(0.45f)), threadId);
}


// Screen filling quad in normalized device coordinates.
constant float2 quadVertices[] = {
    float2(-1, -1),
    float2(-1,  1),
    float2( 1,  1),
    float2(-1, -1),
    float2( 1,  1),
    float2( 1, -1)
};

struct CopyVertexOut {
    float4 position [[position]];
    float2 uv;
};

// Simple vertex shader that passes through NDC quad positions.
vertex CopyVertexOut copyVertex(unsigned short vid [[vertex_id]]) {
    float2 position = quadVertices[vid];

    CopyVertexOut out;

    out.position = float4(position, 0, 1);
    out.uv = position * 0.5f + 0.5f;

    return out;
}

// Simple fragment shader that copies a texture and applies a simple tonemapping function.
fragment float4 copyFragment(CopyVertexOut in [[stage_in]],
                             texture2d<float> tex)
{
    constexpr sampler sam(min_filter::nearest, mag_filter::nearest, mip_filter::none);

    float3 color = tex.sample(sam, in.uv).xyz;

    // Apply a simple tonemapping function to reduce the dynamic range of the
    // input image into a range which the screen can display.
    color = color / (1.0f + color);

    return float4(color, 1.0f);
}

