cbuffer Constants
{
    float4x4 g_WorldViewProj;
    float g_Time; // Tiempo para animar las ondas
    float g_WaveHeight; // Altura de las ondas
    float g_WaveFreq; // Frecuencia de las ondas
    float g_WaveSpeed; // Velocidad de las ondas
};

// Atributos de entrada
struct VSInput
{
    float3 Pos : ATTRIB0;
    float2 UV : ATTRIB1;
};

// Datos de salida
struct PSInput
{
    float4 Pos : SV_POSITION;
    float2 UV : TEX_COORD;
    float3 Color : COLOR; // Color adicional basado en la altura
};

// Función de onda para simular el líquido
float WaveFunction(float2 position, float time)
{
    // Primera onda
    float wave1 = sin(position.x * g_WaveFreq + time * g_WaveSpeed) *
                 sin(position.y * g_WaveFreq * 0.7 + time * g_WaveSpeed * 0.8);
    
    // Segunda onda (en diferente dirección y frecuencia)
    float wave2 = sin(position.x * g_WaveFreq * 0.8 - position.y * g_WaveFreq * 0.6 + time * g_WaveSpeed * 1.3) * 0.5;
    
    // Combinar ondas
    return (wave1 + wave2) * g_WaveHeight;
}

void main(in VSInput VSIn, out PSInput PSIn)
{
    // Aplicar la función de onda a la posición Y del vértice
    float3 position = VSIn.Pos;
    position.y = WaveFunction(position.xz, g_Time);
    
    // Transformar posición final
    PSIn.Pos = mul(float4(position, 1.0), g_WorldViewProj);
    
    // Pasar coordenadas UV
    PSIn.UV = VSIn.UV;
    
    // Color basado en la altura para dar efecto de profundidad
    // Valores entre 0 y 1 para usar como mezcla en el pixel shader
    PSIn.Color = float3(
        0.4 + position.y * 0.6, // Componente R
        0.5 + position.y * 0.5, // Componente G
        0.7 + position.y * 0.3 // Componente B
    );
}