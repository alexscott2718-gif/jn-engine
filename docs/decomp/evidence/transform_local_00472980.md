# transform_local_00472980 evidence

Recovered 2026-07-02 with `tools/ghidra/CreateFunctions.java` against
`~/ghidra-projects/JN_decomp` / `Neutron.exe`.

## Interpretation

The raw decompiler output below misses the first stack argument and therefore
shows the output pointer as an uninitialized local `pfStack_4`. Raw disassembly
fixes the signature: `ret 0x10` and the final stores through `[esp+0x44]` make
the call shape:

```c
float * __thiscall transform_local_00472980(
    void *this, float out4[4], float local_x, float local_y, float local_z);
```

The function calls `this->vtable[+0x328]` to fetch three `OMediaWorldAngle`
components, multiplies each by `.rdata:0048e5e8` (`45.511112f`, `8192/180`) to
index the engine's 14-bit trig table, calls `this->vtable[+0x310]` for world
position, writes a three-axis transformed point to `out4[0..2]`, writes
`out4[3] = 1.0f`, and returns `out4`.

## Raw Ghidra Dump

## transform_local_00472980 @ 00472980

```c

void __thiscall transform_local_00472980(void *this,float param_2,float param_3,float param_4)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  float fVar9;
  float fVar10;
  uint uVar11;
  uint uVar12;
  uint uVar13;
  int iVar14;
  float *pfVar15;
  int iVar16;
  undefined1 auStack_20 [12];
  undefined1 auStack_14 [16];
  float *pfStack_4;

  (**(code **)(*(int *)this + 0x328))(auStack_20);
  uVar11 = __ftol();
  uVar12 = __ftol();
  uVar13 = __ftol();
  iVar14 = (uVar13 & 0x3fff) * 4;
  fVar1 = *(float *)(global_exref + iVar14);
  fVar2 = *(float *)(global_exref + iVar14 + 0x10004);
  iVar14 = (uVar12 & 0x3fff) * 4;
  iVar16 = (uVar11 & 0x3fff) * 4;
  fVar3 = *(float *)(global_exref + iVar14);
  fVar4 = *(float *)(global_exref + iVar14 + 0x10004);
  fVar5 = *(float *)(global_exref + iVar16);
  fVar6 = *(float *)(global_exref + iVar16 + 0x10004);
  fVar10 = fVar6 * param_3 - fVar5 * param_2;
  pfVar15 = (float *)(**(code **)(*(int *)this + 0x310))(auStack_14);
  fVar7 = *pfVar15;
  fVar8 = pfVar15[1];
  fVar9 = pfVar15[2];
  pfStack_4[3] = 1.0;
  *pfStack_4 = fVar7 + fVar10;
  pfStack_4[2] = fVar1 * param_4 + fVar10 * fVar2 + fVar9;
  pfStack_4[1] = ((fVar5 * param_3 + fVar6 * param_2) * fVar4 -
                 (fVar2 * param_4 - fVar10 * fVar1) * fVar3) + fVar8;
  return;
}


```
