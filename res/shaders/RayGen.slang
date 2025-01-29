import Common;

[shader("raygeneration")]
void raygen()
{
    if(pSamps.pass == pSamps.samplesPerPixel) return;
    uint2 pix = DispatchRaysIndex().xy;
    uint2 imgdim = DispatchRaysDimensions().xy;

    //-- define cam
    Ray cam = Ray(pParams.cam.cameraPosition, pParams.cam.cameraForward);
    float3 cx = -normalize(cross(cam.d, abs(cam.d.y) < 0.9 ? float3(0, 1, 0) : float3(0, 0, 1))), cy = cross(cam.d, cx);
    const float2 sdim = float2(0.036, 0.024);

    float S_I = (pParams.cam.S_O * pParams.cam.f) / (pParams.cam.S_O - pParams.cam.f);

    //-- sample sensor
    float2 rnd2 = 2*rand01(uint3(pix, pSamps.pass)).xy;   // vvv tent filter sample  
    float2 tent = float2(rnd2.x<1 ? sqrt(rnd2.x)-1 : 1-sqrt(2-rnd2.x), rnd2.y<1 ? sqrt(rnd2.y)-1 : 1-sqrt(2-rnd2.y));
    float2 s = ((pix + 0.5 * (0.5 + float2((pSamps.pass/2)%2, pSamps.pass%2) + tent)) / float2(imgdim) - 0.5) * sdim;
    float3 spos = cam.o + cx*s.x + cy*s.y, lc = cam.o + cam.d * 0.035;           // sample on 3d sensor plane
    Ray r = Ray(lc, normalize(lc - spos));      // construct ray


    //-- setup lens
    float3 lensP = lc;
    float3 lensN = -cam.d;
    float3 lensX = cross(lensN, float3(0, 1, 0)); // the exact vector doesnt matter
    float3 lensY = cross(lensN, lensX);
    uint3 rndSeed = uint3(pix, pSamps.pass);
    float2 rnd01 = rand01(rndSeed).xy;

    float3 lensSample = lensP + rnd01.x * pParams.cam.A * lensX + rnd01.y * pParams.cam.A * lensY;

    float3 focalPoint = cam.o + (pParams.cam.S_O + S_I) * cam.d;
    float t = dot(focalPoint - r.o, lensN) / dot(r.d, lensN);
    float3 focus = r.o + t * r.d;

    RayDesc rayDesc;
    rayDesc.Origin = lensSample;
    rayDesc.Direction = normalize(focus - lensSample);
    rayDesc.TMin = 0.001;
    rayDesc.TMax = 10000.0;
	
    const uint maxDepth = 12;
    RayPayload payload;
    // initialize accumulated radiance and bxdf
    payload.light=float3(0);
    payload.emissive = 1;
    payload.depth = 1;
    payload.anyHit = false;
    TraceRay(pParams.scene, 0, 0xff, 0, 0, 0, rayDesc, payload);

    if(pSamps.pass == 0) pParams.radianceAccumulator[pix] = float4(0);
    pParams.radianceAccumulator[pix] += float4(payload.light / pSamps.samplesPerPixel, 0);
    pParams.image[pix] = float4(clamp(pParams.radianceAccumulator[pix].xyz, 0, 1), 1);
}
