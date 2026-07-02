// Dump references to explicit addresses and their containing functions.
// @category JNScope
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.ReferenceIterator;

import java.io.PrintWriter;

public class DumpRefs extends GhidraScript {
    @Override
    public void run() throws Exception {
        if (getScriptArgs().length < 2) {
            println("usage: DumpRefs.java <outPath> <addr[:label]> [addr[:label]...]");
            return;
        }

        try (PrintWriter out = new PrintWriter(getScriptArgs()[0])) {
            for (int i = 1; i < getScriptArgs().length; i++) {
                Spec spec = parseSpec(getScriptArgs()[i]);
                Address target = toAddr(spec.address);
                out.println("## " + spec.label + " @ " + target);
                ReferenceIterator refs = currentProgram.getReferenceManager().getReferencesTo(target);
                boolean any = false;
                while (refs.hasNext()) {
                    monitor.checkCancelled();
                    any = true;
                    Reference ref = refs.next();
                    Address from = ref.getFromAddress();
                    Function fn = getFunctionContaining(from);
                    out.println("- from " + from + " type=" + ref.getReferenceType() +
                        " function=" + (fn == null ? "(none)" : fn.getName() + " @ " + fn.getEntryPoint()));
                }
                if (!any) {
                    out.println("- (no refs)");
                }
                out.println();
            }
        }
        println("Wrote " + getScriptArgs()[0]);
    }

    private Spec parseSpec(String raw) throws Exception {
        String[] parts = raw.split(":", 2);
        String addr = parts[0].toLowerCase();
        if (addr.startsWith("0x")) {
            addr = addr.substring(2);
        }
        String label = parts.length > 1 && parts[1].length() > 0 ? parts[1] : addr;
        return new Spec(Long.parseLong(addr, 16), label);
    }

    private static class Spec {
        final long address;
        final String label;
        Spec(long address, String label) {
            this.address = address;
            this.label = label;
        }
    }
}
