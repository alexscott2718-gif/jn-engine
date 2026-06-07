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
import ghidra.program.model.symbol.Reference;
import java.io.FileWriter; import java.io.PrintWriter;

public class Scan_ClassIds extends GhidraScript {
    static String fourcc(long v){
        // little-endian bytes -> chars (memory order)
        char[] c = new char[4];
        for(int i=0;i<4;i++){ int b=(int)((v>>(8*i))&0xff); if(b<32||b>126) return null; c[i]=(char)b; }
        return new String(c);
    }
    @Override public void run() throws Exception {
        PrintWriter out = new PrintWriter(new FileWriter("/tmp/classids.txt"));
        out.println("imm_fourcc  reversed  @site                 function                 nearby_string");
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
                    // look back a few instructions for a string ref (class name)
                    String near="";
                    Instruction p=ins; for(int k=0;k<8 && p!=null;k++){
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
                        f==null?"??":f.getName(), near.length()>40?near.substring(0,40):near);
                }
            }
        }
        out.flush(); out.close(); println("Wrote /tmp/classids.txt");
    }
}
