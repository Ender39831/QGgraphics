#include "Basic.hlsli"

// 顶点着色器(3D)
VertexPosHWNormalTex VS(VertexPosNormalTex vIn)
{
    VertexPosHWNormalTex vOut;
    matrix viewProj = mul(g_View, g_Proj);
    float4 posW = mul(float4(vIn.posL, 1.0f), g_World);

    vOut.posH = mul(posW, viewProj);
    vOut.posW = posW.xyz;
    vOut.normalW = mul(vIn.normalL, (float3x3) g_WorldInvTranspose);
    
    vOut.tex = mul(float4(vIn.tex, 0.0f, 1.0f), g_RotationTex); // 让纹理坐标乘一个旋转矩阵
    
    //vOut.tex = vIn.tex;   // 项目原代码
    return vOut;
}
