import Common;

[shader("miss")]
void miss(inout RayPayload p)
{
    p.light = float3(0, 0, 0);
    p.hit = false;
}