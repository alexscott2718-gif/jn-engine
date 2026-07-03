// Walking-camera recovery pass: repair UpdateWalkingCameraA/B prototypes
// (__thiscall, float dt), re-decompile, dump raw listings for x87 tracing,
// decompile input/vector helpers, and list callers.
// @category JNScope
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileOptions;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.data.FloatDataType;
import ghidra.program.model.data.VoidDataType;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.program.model.listing.Parameter;
import ghidra.program.model.listing.ParameterImpl;
import ghidra.program.model.listing.ReturnParameterImpl;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.ReferenceIterator;
import ghidra.program.model.symbol.SourceType;

import java.io.PrintWriter;

public class WalkCamPass extends GhidraScript {
    @Override
    public void run() throws Exception {
        if (getScriptArgs().length < 1) {
            println("usage: WalkCamPass.java <outPath>");
            return;
        }
        String outPath = getScriptArgs()[0];

        DecompInterface decompiler = new DecompInterface();
        decompiler.setOptions(new DecompileOptions());
        decompiler.openProgram(currentProgram);

        long[] repair = { 0x438bc0L, 0x439900L };

        try (PrintWriter out = new PrintWriter(outPath)) {
            for (long a : repair) {
                Function fn = getFunctionAt(toAddr(a));
                if (fn == null) {
                    out.println("MISSING function @ " + Long.toHexString(a));
                    continue;
                }
                Parameter dt = new ParameterImpl("dt", FloatDataType.dataType, currentProgram);
                fn.updateFunction("__thiscall",
                        new ReturnParameterImpl(VoidDataType.dataType, currentProgram),
                        Function.FunctionUpdateType.DYNAMIC_STORAGE_FORMAL_PARAMS,
                        true, SourceType.USER_DEFINED, dt);
                out.println("repaired " + fn.getName() + " @ " + fn.getEntryPoint()
                        + " -> " + fn.getPrototypeString(false, false));
            }
            out.println();

            long[] dec = { 0x438bc0L, 0x439900L, 0x46a460L, 0x409f60L };
            for (long a : dec) {
                Function fn = getFunctionAt(toAddr(a));
                dumpDecomp(out, decompiler, fn, "0x" + Long.toHexString(a));
            }

            for (Function fn : currentProgram.getFunctionManager().getFunctions(true)) {
                monitor.checkCancelled();
                if (fn.getName().equals("angles")) {
                    dumpDecomp(out, decompiler, fn, "angles-by-name");
                }
            }

            for (long a : repair) {
                dumpListing(out, a);
            }

            for (long a : repair) {
                dumpRefsTo(out, a);
            }
        }
        println("Wrote " + outPath);
    }

    private void dumpDecomp(PrintWriter out, DecompInterface decompiler, Function fn, String label)
            throws Exception {
        if (fn == null) {
            out.println("## " + label + " (function not found)");
            out.println();
            return;
        }
        monitor.checkCancelled();
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

    private void dumpListing(PrintWriter out, long entry) throws Exception {
        Function fn = getFunctionAt(toAddr(entry));
        if (fn == null) {
            return;
        }
        out.println("## LISTING " + fn.getName() + " @ " + fn.getEntryPoint());
        out.println();
        out.println("```asm");
        InstructionIterator it =
                currentProgram.getListing().getInstructions(fn.getBody(), true);
        while (it.hasNext()) {
            monitor.checkCancelled();
            Instruction ins = it.next();
            out.println(ins.getAddress() + "  " + ins.toString());
        }
        out.println("```");
        out.println();
    }

    private void dumpRefsTo(PrintWriter out, long target) throws Exception {
        Address addr = toAddr(target);
        out.println("## REFS TO " + addr);
        ReferenceIterator refs =
                currentProgram.getReferenceManager().getReferencesTo(addr);
        while (refs.hasNext()) {
            monitor.checkCancelled();
            Reference ref = refs.next();
            Address from = ref.getFromAddress();
            Function container =
                    currentProgram.getFunctionManager().getFunctionContaining(from);
            out.println("- " + from + " (" + ref.getReferenceType() + ") in "
                    + (container == null ? "(no function)" : container.getName()
                    + " @ " + container.getEntryPoint()));
        }
        out.println();
    }
}
