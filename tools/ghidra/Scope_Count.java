// Scope the remaining RE surface: function counts, RTTI class names, and which
// functions reference game-logic class strings (C3D*, C-prefixed game classes).
// @category JNScope
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Data;
import ghidra.program.model.listing.DataIterator;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;
import java.io.FileWriter; import java.io.PrintWriter;
import java.util.TreeSet;

public class Scope_Count extends GhidraScript {
    @Override public void run() throws Exception {
        PrintWriter out = new PrintWriter(new FileWriter("/tmp/scope_count.txt"));
        // total defined functions + how many are still FUN_ (un-named/un-analyzed)
        int total=0, unnamed=0, tiny=0, big=0;
        FunctionIterator it = currentProgram.getFunctionManager().getFunctions(true);
        while(it.hasNext()){
            Function f=it.next(); total++;
            if(f.getName().startsWith("FUN_")) unnamed++;
            long len = f.getBody().getNumAddresses();
            if(len<=16) tiny++;
            if(len>=400) big++;
        }
        out.printf("Program: %s%n", currentProgram.getName());
        out.printf("Total defined functions: %d%n", total);
        out.printf("  still FUN_ (unnamed):  %d%n", unnamed);
        out.printf("  tiny (<=16 bytes, thunks/stubs): %d%n", tiny);
        out.printf("  big  (>=400 bytes):    %d%n", big);
        out.println();

        // RTTI class names: ".?AV<Name>@@"
        TreeSet<String> classes = new TreeSet<>();
        TreeSet<String> gameClasses = new TreeSet<>();
        DataIterator di = currentProgram.getListing().getDefinedData(true);
        while(di.hasNext()){
            Data d=di.next(); Object v=d.getValue();
            if(!(v instanceof String)) continue;
            String s=(String)v;
            int i=s.indexOf(".?AV");
            if(i>=0){
                int j=s.indexOf("@@", i);
                String name = (j>i)? s.substring(i+4, j) : s.substring(i+4);
                classes.add(name);
                // game-logic heuristic: starts with C and a digit, or C + capital (gameplay) but not OMedia
                if(!name.startsWith("OMedia") && (name.startsWith("C")))
                    gameClasses.add(name);
            }
        }
        out.printf("Total RTTI classes (.?AV..@@): %d%n", classes.size());
        out.printf("Game-logic 'C*' classes (non-OMedia): %d%n", gameClasses.size());
        out.println("---- C* game classes ----");
        for(String c: gameClasses) out.println("  "+c);
        out.println();
        out.println("---- ALL RTTI classes ----");
        for(String c: classes) out.println("  "+c);
        out.flush(); out.close();
        println("Wrote /tmp/scope_count.txt total="+total+" classes="+classes.size());
    }
}
