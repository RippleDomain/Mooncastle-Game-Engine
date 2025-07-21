struct PostProcessConstants
{
    uint GPassMainBufferIndex;
};

ConstantBuffer<PostProcessConstants> ShaderParams : register(b1);

float4 PostProcessPS(in noperspective float4 Position : SV_Position, in noperspective float2 UV : TEXCOORD) : SV_Target0
{
    Texture2D gPassMain = ResourceDescriptorHeap[ShaderParams.GPassMainBufferIndex];
    float4 color = float4(gPassMain[Position.xy].xyz, 1.f);
    
    return color;
}