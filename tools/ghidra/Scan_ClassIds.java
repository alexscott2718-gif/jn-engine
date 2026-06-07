// Recover the FourCC<->class map: scan Neutron.exe for class-id registration
// immediates (the InitObject pattern `(*(this->vtbl+0xc4))(0x33XXXXXX)`), where the
// 4-byte little-endian immediate spells a "3XXX"/printable FourCC. Report the
// containing function and any nearby class-name string for identification.
// @category JNScope
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.program.model.scalar.Scalar;
import ghidra.program.model.mem.MemoryBlock;
import ghidra.program.model.symbol.Namespace;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.Symbol;
import ghidra.program.model.symbol.SymbolIterator;
import ghidra.program.model.symbol.SymbolTable;
import java.io.FileWriter; import java.io.PrintWriter;
import java.util.HashMap;
import java.util.Map;

public class Scan_ClassIds extends GhidraScript {
    private static final String AMBIGUOUS = "<ambiguous>";

    static String fourcc(long v){
        // little-endian bytes -> chars (memory order)
        char[] c = new char[4];
        for(int i=0;i<4;i++){ int b=(int)((v>>(8*i))&0xff); if(b<32||b>126) return null; c[i]=(char)b; }
        return new String(c);
    }

    private boolean isExecutableAddress(Address address) {
        MemoryBlock block = currentProgram.getMemory().getBlock(address);
        return block != null && block.isExecute();
    }

    private Address readPointer(Address address) {
        try {
            long value = currentProgram.getMemory().getInt(address) & 0xffffffffL;
            return toAddr(value);
        }
        catch (Exception e) {
            return null;
        }
    }

    private Map<Address, String> buildVtableOwnerMap() throws Exception {
        Map<Address, String> owners = new HashMap<>();
        SymbolTable symbols = currentProgram.getSymbolTable();
        SymbolIterator it = symbols.getAllSymbols(true);
        while (it.hasNext()) {
            monitor.checkCancelled();
            Symbol sym = it.next();
            String name = sym.getName();
            if (!name.equals("vftable") && !name.startsWith("vftable_")) {
                continue;
            }
            Namespace ns = sym.getParentNamespace();
            if (ns == null || ns.getName() == null || !ns.getName().startsWith("C")) {
                continue;
            }
            String owner = ns.getName();
            Address vftable = sym.getAddress();
            for (int slot = 0; slot < 512; slot++) {
                monitor.checkCancelled();
                Address target = readPointer(vftable.add(slot * 4L));
                if (target == null || !isExecutableAddress(target)) {
                    break;
                }
                Function fn = getFunctionContaining(target);
                if (fn == null) {
                    continue;
                }
                Address entry = fn.getEntryPoint();
                String previous = owners.get(entry);
                if (previous == null) {
                    owners.put(entry, owner);
                }
                else if (!previous.equals(owner)) {
                    owners.put(entry, AMBIGUOUS);
                }
            }
        }
        return owners;
    }

    @Override public void run() throws Exception {
        Map<Address, String> vtableOwners = buildVtableOwnerMap();
        PrintWriter out = new PrintWriter(new FileWriter("/tmp/classids.txt"));
        out.println("imm_fourcc  reversed  @site                 function                 class_or_nearby_string");
        InstructionIterator it = currentProgram.getListing().getInstructions(true);
        while(it.hasNext()){
            Instruction ins=it.next();
            if(!ins.getMnemonicString().toUpperCase().startsWith("PUSH")) continue;
            for(int i=0;i<ins.getNumOperands();i++){
                for(Object o: ins.getOpObjects(i)){
                    if(!(o instanceof Scalar)) continue;
                    long v=((Scalar)o).getUnsignedValue();
                    if(v>0xffffffffL || v<0x20202020L) continue;
                    String fc=fourcc(v); if(fc==null) continue;
                    // require it to contain a digit '3' or look like a tag (heuristic: starts with '3' or all upper)
                    boolean tagish = fc.charAt(0)=='3' || fc.matches("[A-Z0-9]{4}");
                    if(!tagish) continue;
                    String rev = new StringBuilder(fc).reverse().toString();
                    Function f=getFunctionContaining(ins.getAddress());
                    String func = "??";
                    String owner = "";
                    if (f != null) {
                        func = String.format("FUN_%08x", f.getEntryPoint().getOffset());
                        Namespace ns = f.getParentNamespace();
                        if (ns != null && ns.getName() != null &&
                            ns.getName().startsWith("C")) {
                            owner = ns.getName();
                        }
                        if (owner.isEmpty()) {
                            String fromVtable = vtableOwners.get(f.getEntryPoint());
                            if (fromVtable != null && !AMBIGUOUS.equals(fromVtable)) {
                                owner = fromVtable;
                            }
                        }
                    }
                    // look back a few instructions for a string ref (class name)
                    String near="";
                    if (!owner.isEmpty()) near = owner;
                    Instruction p=ins; for(int k=0;k<8 && p!=null;k++){
                        if (!near.isEmpty()) break;
                        Reference[] rs=p.getReferencesFrom();
                        for(Reference r: rs){
                            Address to=r.getToAddress();
                            ghidra.program.model.listing.Data d=getDataAt(to);
                            if(d!=null && d.getValue() instanceof String){
                                String s=(String)d.getValue();
                                if(s.contains("C3D")||s.contains("::")||s.contains("ASE")||s.contains(".ase")){ near=s; break; }
                            }
                        }
                        if(!near.isEmpty()) break;
                        p=p.getPrevious();
                    }
                    out.printf("%-9s  %-8s  @%s  %-24s  %s%n", fc, rev, ins.getAddress(),
                        func, near.length()>40?near.substring(0,40):near);
                }
            }
        }
        out.flush(); out.close(); println("Wrote /tmp/classids.txt");
    }
}
