StructuredBuffer<int>   src_buffer  : register(t0);
RWStructuredBuffer<int> dest_buffer : register(u0);

[numthreads(64,1,1)]
void main(uint3 group_id : SV_GroupID, uint3 thread_id : SV_DispatchThreadID, uint3 group_thread_id : SV_GroupThreadID, uint group_index : SV_GroupIndex)
{
    const int index = thread_id.x;

    dest_buffer[index] = src_buffer[index] + index;
}
