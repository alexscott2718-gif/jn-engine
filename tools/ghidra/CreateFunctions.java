// Create and decompile functions at explicit addresses.
// @category JNScope
import ghidra.app.cmd.disassemble.DisassembleCommand;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileOptions;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.SourceType;

import java.io.PrintWriter;

public class CreateFunctions extends GhidraScript {
    @Override
    public void run() throws Exception {
        if (getScriptArgs().length < 2) {
            println("usage: CreateFunctions.java <outPath> <addr[:name]> [addr[:name]...]");
            return;
        }

        String outPath = getScriptArgs()[0];
        DecompInterface decompiler = new DecompInterface();
        decompiler.setOptions(new DecompileOptions());
        decompiler.openProgram(currentProgram);

        try (PrintWriter out = new PrintWriter(outPath)) {
            for (int i = 1; i < getScriptArgs().length; i++) {
                monitor.checkCancelled();
                Spec spec = parseSpec(getScriptArgs()[i]);
                Address entry = toAddr(spec.address);

                Function fn = getFunctionAt(entry);
                if (fn == null) {
                    new DisassembleCommand(entry, null, true).applyTo(currentProgram, monitor);
                    fn = createFunction(entry, spec.name);
                    println("Created function " + spec.name + " @ " + entry);
                } else if (!fn.getName().equals(spec.name)) {
                    fn.setName(spec.name, SourceType.USER_DEFINED);
                    println("Renamed function @ " + entry + " to " + spec.name);
                }

                if (fn != null) {
                    fn.setCallingConvention("__thiscall");
                }

                out.println("## " + (fn == null ? spec.name : fn.getName()) + " @ " + entry);
                out.println();
                out.println("```c");
                if (fn == null) {
                    out.println("(function creation failed)");
                } else {
                    DecompileResults res = decompiler.decompileFunction(fn, 90, monitor);
                    if (res != null && res.getDecompiledFunction() != null) {
                        out.println(res.getDecompiledFunction().getC());
                    } else {
                        out.println("(decompile failed)");
                    }
                }
                out.println("```");
                out.println();
            }
        }

        currentProgram.flushEvents();
        println("Wrote " + outPath);
    }

    private Spec parseSpec(String raw) throws Exception {
        String[] parts = raw.split(":", 2);
        String addr = parts[0].toLowerCase();
        if (addr.startsWith("0x")) {
            addr = addr.substring(2);
        }
        String name = parts.length > 1 && parts[1].length() > 0
            ? parts[1]
            : "FUN_" + addr;
        return new Spec(Long.parseLong(addr, 16), name);
    }

    private static class Spec {
        final long address;
        final String name;
        Spec(long address, String name) {
            this.address = address;
            this.name = name;
        }
    }
}
