// Decompile arbitrary functions by address or symbol name.
// @category JNScope
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileOptions;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;

import java.io.PrintWriter;

public class DumpFunctions extends GhidraScript {
    @Override
    public void run() throws Exception {
        if (getScriptArgs().length < 2) {
            println("usage: DumpFunctions.java <outPath> <addrOrName> [addrOrName...]");
            return;
        }

        DecompInterface decompiler = new DecompInterface();
        decompiler.setOptions(new DecompileOptions());
        decompiler.openProgram(currentProgram);

        String outPath = getScriptArgs()[0];
        try (PrintWriter out = new PrintWriter(outPath)) {
            for (int i = 1; i < getScriptArgs().length; i++) {
                monitor.checkCancelled();
                Function fn = resolve(getScriptArgs()[i]);
                if (fn == null) {
                    out.println("## " + getScriptArgs()[i]);
                    out.println();
                    out.println("(function not found)");
                    out.println();
                    continue;
                }
                out.println("## " + fn.getName() + " @ " + fn.getEntryPoint());
                out.println();
                out.println("```c");
                DecompileResults res = decompiler.decompileFunction(fn, 90, monitor);
                if (res != null && res.getDecompiledFunction() != null) {
                    out.println(res.getDecompiledFunction().getC());
                }
                else {
                    out.println("(decompile failed)");
                }
                out.println("```");
                out.println();
            }
        }
        println("Wrote " + outPath);
    }

    private Function resolve(String spec) {
        try {
            String s = spec.toLowerCase();
            if (s.startsWith("0x")) {
                s = s.substring(2);
            }
            if (s.matches("[0-9a-f]+")) {
                Address address = toAddr(Long.parseLong(s, 16));
                Function fn = getFunctionAt(address);
                return fn != null ? fn : getFunctionContaining(address);
            }
        }
        catch (Exception ignored) {
        }
        return getGlobalFunctions(spec).isEmpty() ? null : getGlobalFunctions(spec).get(0);
    }
}
