#ifndef LIGHTING_H
#define LIGHTING_H

#define PI 3.1415926535f
#define INV_PI 0.318309f
#define EIGHTH 0.125f
#define MIN_VALUE 0.000001f

float distribution_ggx(float n_dot_h, float roughness)
{
    float a     = roughness * roughness;
    float a2    = a * a;
    float denom = n_dot_h * n_dot_h * (a2 - 1) + 1;
    denom       = PI * denom * denom;
    denom       = max(denom, MIN_VALUE);
    denom       = 1.f / denom;

    return a2 * denom;
}

float geometry_smith(float n_dot_v, float n_dot_l, float roughness)
{
    float r     = roughness + 1;
    float k     = (r * r) * EIGHTH;
    float gg_x1 = n_dot_v / (n_dot_v * (1 - k) + k); // schlick
    float gg_x2 = n_dot_l / (n_dot_l * (1 - k) + k);

    return gg_x1 + gg_x2;
}

float3 fresnel_schlick(float h_dot_v, float3 base_reflectivity)
{
    return base_reflectivity + (1 - base_reflectivity) * pow(1 - h_dot_v, 5);
}

float3 calculate_lighting(float3 position, float3 normal, float3 light_pos, float3 light_color, float3 ambient_color, float3 albedo, float3 camera_position, float roughness, float metallic)
{
    float3 base_reflectivity = albedo * metallic;

    float3 L = normalize(light_pos - position);
    float3 V = normalize(camera_position - position);
    float3 H = normalize(V + L);

    float n_dot_v = max(dot(normal, V), MIN_VALUE);
    float n_dot_l = max(dot(normal, L), MIN_VALUE);
    float n_dot_h = max(dot(normal, H), MIN_VALUE);
    float h_dot_v = max(dot(H, V), MIN_VALUE);

    float D  = distribution_ggx(n_dot_h, roughness);
    float G  = geometry_smith(n_dot_v, n_dot_l, roughness);
    float3 F = fresnel_schlick(h_dot_v, base_reflectivity);

    float3 specular = D * G * F;
    specular *= 0.25f * n_dot_v * n_dot_l;

    float3 KD = float3(1,1,1) - F;
    KD *= 1 - metallic;

    return (KD * albedo * INV_PI + specular) * n_dot_l * light_color;
}

#endif
