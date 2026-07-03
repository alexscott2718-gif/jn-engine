// Walking-camera recovery pass 2: repair ProjectNoisyCameraTarget /
// ProbePlayerRayBlend prototypes, re-decompile, dump ProjectNoisy listing,
// decompile the ray helper, and identify the import at IAT 0x0048d108
// (OMedia3DVector::angles call target).
// @category JNScope
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileOptions;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.data.DataType;
import ghidra.program.model.data.FloatDataType;
import ghidra.program.model.data.PointerDataType;
import ghidra.program.model.listing.Data;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.program.model.listing.Parameter;
import ghidra.program.model.listing.ParameterImpl;
import ghidra.program.model.listing.ReturnParameterImpl;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.Symbol;
import ghidra.program.model.symbol.SourceType;

import java.io.PrintWriter;

public class WalkCamPass2 extends GhidraScript {
    @Override
    public void run() throws Exception {
        String outPath = getScriptArgs()[0];

        DecompInterface decompiler = new DecompInterface();
        decompiler.setOptions(new DecompileOptions());
        decompiler.openProgram(currentProgram);

        DataType f = FloatDataType.dataType;
        DataType pf = new PointerDataType(f);

        try (PrintWriter out = new PrintWriter(outPath)) {
            // ProjectNoisyCameraTarget: float* (float* out4, float x, float y, float z)
            Function noisy = getFunctionAt(toAddr(0x43a5d0L));
            noisy.updateFunction("__thiscall",
                    new ReturnParameterImpl(pf, currentProgram),
                    Function.FunctionUpdateType.DYNAMIC_STORAGE_FORMAL_PARAMS,
                    true, SourceType.USER_DEFINED,
                    new ParameterImpl("out4", pf, currentProgram),
                    new ParameterImpl("x", f, currentProgram),
                    new ParameterImpl("y", f, currentProgram),
                    new ParameterImpl("z", f, currentProgram));
            out.println("repaired " + noisy.getPrototypeString(false, false));

            // ProbePlayerRayBlend: float (vec4 by value = 4 floats, float* eye)
            Function probe = getFunctionAt(toAddr(0x43b820L));
            probe.updateFunction("__thiscall",
                    new ReturnParameterImpl(f, currentProgram),
                    Function.FunctionUpdateType.DYNAMIC_STORAGE_FORMAL_PARAMS,
                    true, SourceType.USER_DEFINED,
                    new ParameterImpl("src_x", f, currentProgram),
                    new ParameterImpl("src_y", f, currentProgram),
                    new ParameterImpl("src_z", f, currentProgram),
                    new ParameterImpl("src_w", f, currentProgram),
                    new ParameterImpl("eye4", pf, currentProgram));
            out.println("repaired " + probe.getPrototypeString(false, false));
            out.println();

            long[] dec = { 0x43a5d0L, 0x43b820L, 0x47c210L };
            for (long a : dec) {
                Function fn = getFunctionAt(toAddr(a));
                if (fn == null) {
                    out.println("## 0x" + Long.toHexString(a) + " (function not found)");
                    out.println();
                    continue;
                }
                out.println("## " + fn.getName() + " @ " + fn.getEntryPoint());
                out.println();
                out.println("```c");
                DecompileResults res = decompiler.decompileFunction(fn, 120, monitor);
                if (res != null && res.getDecompiledFunction() != null) {
                    out.println(res.getDecompiledFunction().getC());
                } else {
                    out.println("(decompile failed)");
                }
                out.println("```");
                out.println();
            }

            // listing of ProjectNoisyCameraTarget for x87 verification
            out.println("## LISTING ProjectNoisyCameraTarget @ 0043a5d0");
            out.println();
            out.println("```asm");
            InstructionIterator it =
                    currentProgram.getListing().getInstructions(noisy.getBody(), true);
            while (it.hasNext()) {
                monitor.checkCancelled();
                Instruction ins = it.next();
                out.println(ins.getAddress() + "  " + ins.toString());
            }
            out.println("```");
            out.println();

            // identify the import slot 0x0048d108
            Address iat = toAddr(0x48d108L);
            out.println("## IAT 0x0048d108");
            Data d = currentProgram.getListing().getDataAt(iat);
            out.println("data: " + (d == null ? "(none)" : d.toString()));
            for (Symbol s : currentProgram.getSymbolTable().getSymbols(iat)) {
                out.println("symbol: " + s.getName(true));
            }
            Reference[] refs = currentProgram.getReferenceManager().getReferencesFrom(iat);
            for (Reference r : refs) {
                out.println("ref -> " + r.getToAddress() + " "
                        + currentProgram.getSymbolTable().getPrimarySymbol(r.getToAddress()));
            }
        }
        println("Wrote " + outPath);
    }
}
