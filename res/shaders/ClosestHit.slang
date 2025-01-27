import Common;


[shader("closesthit")]
void closestHit(inout RayPayload hitValue, in BuiltInTriangleIntersectionAttributes attr)
{
    hitValue.hit = true;
    // todo: replace with anyhit shader
    if(hitValue.anyHit)
        return;
    const float3 barycentricCoords = float3(1.0f - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);

    ModelReference m = pParams.modelData[InstanceID()];

    // offset into the index buffer
    uint indexOffset = m.indicesOffset;
    // added to indices to reference correct part of global mesh pool
    uint vertexOffset = m.positionOffset;

    uint vertexIndex0 = vertexOffset + pParams.indexBuffer[indexOffset + 3 * PrimitiveIndex() + 0];
    uint vertexIndex1 = vertexOffset + pParams.indexBuffer[indexOffset + 3 * PrimitiveIndex() + 1];
    uint vertexIndex2 = vertexOffset + pParams.indexBuffer[indexOffset + 3 * PrimitiveIndex() + 2];

    Vertex attr0 = loadVertex(vertexIndex0);
    Vertex attr1 = loadVertex(vertexIndex1);
    Vertex attr2 = loadVertex(vertexIndex2);

    Vertex vert = Vertex.interpolate(attr0, attr1, attr2, barycentricCoords);

    float3 normalLight = dot(vert.normal, WorldRayDirection()) < 0 ? vert.normal : -vert.normal;

    MaterialParameter mat; // TOOD:
    
    hitValue.depth++;
    float3 localAccRad = float3(0);
    float3 rnd = rand01(uint3(vertexIndex0, vertexIndex1, vertexIndex2));
    //float kt = ka + ks;
    //float s = -log(rnd.z) / kt;
    //float3 xs = r.o + s * r.d;
    //if (s < t) {
    //    float p = kt * rnd.z;
    //    if (depth > 5) {
    //        if (rnd.z >= p) break;
    //        else accmat /= p;
    //    }
    //    float3 ldirect = nextEventEstimation(accmat, r.d, xs, -r.d, kt, true, rnd);
    //    accrad += (fogEmm + ks * ldirect) / kt;
    //    accmat *= ks / kt;
    //    rayDesc.Origin = xs;
    //    rayDesc.Direction = float3(
    //        cos(2*PI*rnd.x)*sqrt(1-rnd.y*rnd.y),
    //        sin(2*PI*rnd.x)*sqrt(1-rnd.y*rnd.y),
    //        rnd.y
    //    );
    //    continue;
    //}

    //float p = max(max(mat.albedo.x, mat.albedo.y), mat.albedo.z);
    //if(hitValue.depth > 5) {
    //    if (rnd.z >= p) return;
    //    else hitValue.accmat /= p;
    //}


    //-- Ideal DIFFUSE reflection
    //if(bool(useNEE)) {
    //    accrad += nextEventEstimation(accmat, r.d, params.x, params.nl, kt, false, rnd);
    //}
    for(uint i = 0; i < pSamps.numDirectionalLights; ++i) {
        float3 x = vert.position;
        float3 l = -pParams.directionalLights[i].direction.xyz;
        RayDesc rayDesc;
        rayDesc.TMax = 10000.0f;
        rayDesc.TMin = 0.001f;
        rayDesc.Origin = x;
        rayDesc.Direction = l;
        RayPayload payload;
        payload.depth = hitValue.depth;
        payload.emissive = 1;
        payload.anyHit = true;
        TraceRay(pParams.scene, 0, 0xff, 0, 0, 0, rayDesc, payload);

        // we have missed all geometry, so directional light is affecting us
        if(!payload.hit) {
            localAccRad += mat.shade(vert.normal, -WorldRayDirection(), -pParams.directionalLights[i].direction, pParams.directionalLights[i].color);
        }
    }
    for(uint i = 0; i < pSamps.numPointLights; ++i) {
        RayPayload payload;
        float3 x = vert.position;
        float3 l = pParams.pointLights[i].position - vert.position;
        // todo: cancel if light too far away to affect

        RayDesc rayDesc;
        rayDesc.TMax = 1.0f;
        rayDesc.TMin = 0.001f;
        rayDesc.Origin = x;
        rayDesc.Direction = l;
        TraceRay(pParams.scene, 0, 0xff, 0, 0, 0, rayDesc, payload);
    
        // hitting only after the light
        if(!payload.hit) {
            localAccRad += mat.shade(vert.normal, -WorldRayDirection(), normalize(l), pParams.pointLights[i].color);
        }
    }
    hitValue.light += localAccRad;
    // Indirect Illumination: cosine-weighted importance sampling
    if(hitValue.depth < 12) {
    	float r1 = 2 * PI * rnd.x, r2 = rnd.y, r2s = sqrt(r2);
        float3 w = normalLight;
        float3 u = normalize((cross(abs(w.x)>0.1 ? float3(0,1,0) : float3(1,0,0), w)));
        float3 v = cross(w,u);
        RayDesc rayDesc;
        rayDesc.TMax = 10000.0f;
        rayDesc.TMin = 0.001f;
        rayDesc.Origin = vert.position;
        rayDesc.Direction = normalize(u*cos(r1)*r2s + v * sin(r1)*r2s + w * sqrt(1 - r2));
        RayPayload payload;
        payload.light = float3(0);
        payload.emissive = 0;   // in the next bounce, consider reflective part only!
        payload.depth = hitValue.depth+1;
        payload.anyHit = false;
        TraceRay(pParams.scene, 0, 0xff, 0, 0, 0, rayDesc, payload);
    }
}