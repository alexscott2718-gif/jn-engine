// Seed Ghidra class namespaces, class structs, vftable labels, and thiscall
// signatures from docs/decomp/_hierarchy.md.
//
// This is intentionally conservative: when the same vtable target appears in
// several derived classes, ownership is assigned to the shallowest class in the
// recovered base chain so inherited methods stay on their base when possible.
//
// Run without -readOnly so the project saves:
//   analyzeHeadless ~/ghidra-projects JN_decomp -process Neutron.exe -noanalysis \
//     -scriptPath tools/ghidra -postScript ApplyRttiClassMarkup.java \
//     docs/decomp/_hierarchy.md docs/decomp/_ghidra_markup.md
//
// @category JNScope

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.data.CategoryPath;
import ghidra.program.model.data.DataTypeConflictHandler;
import ghidra.program.model.data.PointerDataType;
import ghidra.program.model.data.Structure;
import ghidra.program.model.data.StructureDataType;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Function.FunctionUpdateType;
import ghidra.program.model.listing.Parameter;
import ghidra.program.model.listing.ParameterImpl;
import ghidra.program.model.listing.ReturnParameterImpl;
import ghidra.program.model.mem.MemoryAccessException;
import ghidra.program.model.mem.MemoryBlock;
import ghidra.program.model.symbol.Namespace;
import ghidra.program.model.symbol.SourceType;
import ghidra.program.model.symbol.Symbol;
import ghidra.program.model.symbol.SymbolTable;
import ghidra.program.model.lang.CompilerSpec;
import ghidra.util.exception.InvalidInputException;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.PrintWriter;
import java.time.LocalDate;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeMap;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class ApplyRttiClassMarkup extends GhidraScript {
    private static final Pattern BACKTICK = Pattern.compile("`([^`]+)`");
    private static final CategoryPath CLASS_PATH =
        new CategoryPath(CategoryPath.ROOT, "JNDecompClasses");

    private static class ClassInfo {
        String name;
        int baseDepth;
        List<Address> vftables = new ArrayList<>();
        Namespace namespace;
        Structure struct;
    }

    private static class MethodRef implements Comparable<MethodRef> {
        ClassInfo owner;
        int vtableIndex;
        int slot;

        @Override
        public int compareTo(MethodRef other) {
            int byDepth = Integer.compare(owner.baseDepth, other.owner.baseDepth);
            if (byDepth != 0) {
                return byDepth;
            }
            int byName = owner.name.compareTo(other.owner.name);
            if (byName != 0) {
                return byName;
            }
            int byVtable = Integer.compare(vtableIndex, other.vtableIndex);
            if (byVtable != 0) {
                return byVtable;
            }
            return Integer.compare(slot, other.slot);
        }
    }

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        String hierarchyPath = args.length > 0 ? args[0] :
            "/home/scotty/jn-engine/docs/decomp/_hierarchy.md";
        String reportPath = args.length > 1 ? args[1] :
            "/home/scotty/jn-engine/docs/decomp/_ghidra_markup.md";

        Map<String, ClassInfo> classes = readHierarchy(hierarchyPath);
        createNamespacesAndStructs(classes);
        int vftableLabels = labelVftables(classes);

        Map<Address, MethodRef> owners = chooseMethodOwners(classes);
        MarkupCounts counts = applyMethodMarkup(owners);
        writeReport(reportPath, hierarchyPath, classes, owners, vftableLabels, counts);

        println("Applied RTTI class markup: classes=" + classes.size() +
            " vftables=" + vftableLabels +
            " methods_seen=" + owners.size() +
            " methods_updated=" + counts.updated);
    }

    private Map<String, ClassInfo> readHierarchy(String path) throws Exception {
        Map<String, ClassInfo> classes = new TreeMap<>();
        try (BufferedReader reader = new BufferedReader(new FileReader(path))) {
            String line;
            while ((line = reader.readLine()) != null) {
                monitor.checkCancelled();
                if (!line.startsWith("| `")) {
                    continue;
                }

                String[] cols = line.split("\\|", -1);
                if (cols.length < 4) {
                    continue;
                }

                List<String> classTokens = extractBacktickTokens(cols[1]);
                if (classTokens.isEmpty()) {
                    continue;
                }

                ClassInfo info = new ClassInfo();
                info.name = classTokens.get(0);
                info.baseDepth = extractBacktickTokens(cols[2]).size();
                for (String token : extractBacktickTokens(cols[3])) {
                    if (token.matches("[0-9a-fA-F]{8}")) {
                        info.vftables.add(toAddr(token));
                    }
                }
                classes.put(info.name, info);
            }
        }
        return classes;
    }

    private List<String> extractBacktickTokens(String text) {
        List<String> tokens = new ArrayList<>();
        Matcher matcher = BACKTICK.matcher(text);
        while (matcher.find()) {
            tokens.add(matcher.group(1));
        }
        return tokens;
    }

    private void createNamespacesAndStructs(Map<String, ClassInfo> classes) throws Exception {
        SymbolTable symbols = currentProgram.getSymbolTable();
        for (ClassInfo info : classes.values()) {
            monitor.checkCancelled();
            Namespace namespace = symbols.getNamespace(info.name, null);
            if (namespace == null) {
                namespace = symbols.createClass(null, info.name, SourceType.ANALYSIS);
            }
            info.namespace = namespace;

            StructureDataType struct = new StructureDataType(CLASS_PATH, info.name, 4,
                currentProgram.getDataTypeManager());
            struct.setDescription("Seeded from Neutron.exe RTTI by ApplyRttiClassMarkup.java");
            struct.replaceAtOffset(0, new PointerDataType(), 4, "vftable",
                "Primary vftable pointer; fields are filled during per-class RE.");
            info.struct = (Structure) currentProgram.getDataTypeManager()
                .addDataType(struct, DataTypeConflictHandler.REPLACE_HANDLER);
        }
    }

    private int labelVftables(Map<String, ClassInfo> classes) throws Exception {
        int count = 0;
        for (ClassInfo info : classes.values()) {
            for (int i = 0; i < info.vftables.size(); i++) {
                monitor.checkCancelled();
                String label = i == 0 ? "vftable" : "vftable_" + i;
                createNewLabel(info.vftables.get(i), label, info.namespace, SourceType.ANALYSIS);
                count++;
            }
        }
        return count;
    }

    private Map<Address, MethodRef> chooseMethodOwners(Map<String, ClassInfo> classes)
            throws Exception {
        Map<Address, List<MethodRef>> refsByTarget = new HashMap<>();
        for (ClassInfo info : classes.values()) {
            for (int vti = 0; vti < info.vftables.size(); vti++) {
                Address vftable = info.vftables.get(vti);
                int slot = 0;
                while (slot < 512) {
                    monitor.checkCancelled();
                    Address target = readPointer(vftable.add(slot * 4L));
                    if (target == null || !isExecutableAddress(target)) {
                        break;
                    }
                    Function fn = getFunctionContaining(target);
                    if (fn != null) {
                        MethodRef ref = new MethodRef();
                        ref.owner = info;
                        ref.vtableIndex = vti;
                        ref.slot = slot;
                        refsByTarget.computeIfAbsent(fn.getEntryPoint(), k -> new ArrayList<>())
                            .add(ref);
                    }
                    slot++;
                }
            }
        }

        Map<Address, MethodRef> owners = new TreeMap<>();
        for (Map.Entry<Address, List<MethodRef>> entry : refsByTarget.entrySet()) {
            List<MethodRef> refs = entry.getValue();
            Collections.sort(refs);
            owners.put(entry.getKey(), refs.get(0));
        }
        return owners;
    }

    private static class MarkupCounts {
        int updated;
        int renamed;
        int thiscall;
        int paramTyped;
        int skipped;
    }

    private MarkupCounts applyMethodMarkup(Map<Address, MethodRef> owners) throws Exception {
        MarkupCounts counts = new MarkupCounts();
        Set<Address> seen = new HashSet<>();
        for (Map.Entry<Address, MethodRef> entry : owners.entrySet()) {
            monitor.checkCancelled();
            Address entryPoint = entry.getKey();
            if (!seen.add(entryPoint)) {
                continue;
            }

            Function fn = getFunctionAt(entryPoint);
            if (fn == null) {
                counts.skipped++;
                continue;
            }

            MethodRef ref = entry.getValue();
            boolean changed = false;

            if (fn.getName().startsWith("FUN_") || fn.getName().startsWith("thunk_FUN_")) {
                String name = String.format("vfunc_%02d_%03d", ref.vtableIndex, ref.slot);
                createNewLabel(entryPoint, name, ref.owner.namespace, SourceType.ANALYSIS);
                counts.renamed++;
                changed = true;
            }

            if (!CompilerSpec.CALLING_CONVENTION_thiscall.equals(fn.getCallingConventionName())) {
                ReturnParameterImpl ret =
                    new ReturnParameterImpl(fn.getSignature().getReturnType(), currentProgram);
                fn.updateFunction(CompilerSpec.CALLING_CONVENTION_thiscall, ret,
                    FunctionUpdateType.DYNAMIC_STORAGE_ALL_PARAMS, true, SourceType.ANALYSIS,
                    fn.getParameters());
                counts.thiscall++;
                changed = true;
            }

            Parameter[] params = fn.getParameters();
            Parameter[] newParams;
            if (params.length == 0) {
                newParams = new Parameter[1];
            }
            else {
                newParams = params;
            }
            newParams[0] = new ParameterImpl("this", new PointerDataType(ref.owner.struct),
                currentProgram);
            fn.replaceParameters(FunctionUpdateType.DYNAMIC_STORAGE_ALL_PARAMS, true,
                SourceType.ANALYSIS, newParams);
            counts.paramTyped++;
            changed = true;

            if (changed) {
                counts.updated++;
            }
        }
        return counts;
    }

    private void writeReport(String reportPath, String hierarchyPath, Map<String, ClassInfo> classes,
            Map<Address, MethodRef> owners, int vftableLabels, MarkupCounts counts)
            throws Exception {
        File report = new File(reportPath);
        File parent = report.getParentFile();
        if (parent != null) {
            parent.mkdirs();
        }

        try (PrintWriter out = new PrintWriter(new FileWriter(report))) {
            out.println("# Ghidra RTTI Markup Report");
            out.println();
            out.println("> Generated by `tools/ghidra/ApplyRttiClassMarkup.java` on " +
                LocalDate.now() + ".");
            out.println("> Input hierarchy: `" + hierarchyPath + "`.");
            out.println();
            out.println("## Summary");
            out.println();
            out.println("| Metric | Count |");
            out.println("|---|---:|");
            out.println("| Class namespaces / seed structs | " + classes.size() + " |");
            out.println("| Vftable labels applied | " + vftableLabels + " |");
            out.println("| Unique vtable target functions seen | " + owners.size() + " |");
            out.println("| Functions updated | " + counts.updated + " |");
            out.println("| Functions renamed from `FUN_*` | " + counts.renamed + " |");
            out.println("| Functions set to `__thiscall` | " + counts.thiscall + " |");
            out.println("| Functions with typed `this` param | " + counts.paramTyped + " |");
            out.println("| Functions skipped | " + counts.skipped + " |");
            out.println();
            out.println("## Caveats");
            out.println();
            out.println("- Seed structs contain only the primary `vftable` pointer. Field offsets are " +
                "filled during per-class RE from `.gam` registrars and decompiler access maps.");
            out.println("- Vtable target ownership is heuristic: inherited functions are assigned to " +
                "the shallowest class that references the target.");
            out.println("- Constructors, destructors, and non-virtual helpers still need class-specific " +
                "confirmation.");
        }
    }

    private boolean isExecutableAddress(Address address) {
        if (address == null) {
            return false;
        }
        MemoryBlock block = currentProgram.getMemory().getBlock(address);
        return block != null && block.isExecute();
    }

    private Address readPointer(Address address) throws MemoryAccessException {
        byte[] bytes = new byte[4];
        currentProgram.getMemory().getBytes(address, bytes);
        long value = ((long) bytes[0] & 0xffL) |
            (((long) bytes[1] & 0xffL) << 8) |
            (((long) bytes[2] & 0xffL) << 16) |
            (((long) bytes[3] & 0xffL) << 24);
        if (value == 0) {
            return null;
        }
        return currentProgram.getAddressFactory().getDefaultAddressSpace().getAddress(value);
    }

    private void createNewLabel(Address address, String name, Namespace namespace,
            SourceType sourceType) {
        try {
            Symbol existing = getSymbolAt(address);
            if (existing != null && existing.getSource() != SourceType.DEFAULT) {
                return;
            }
            currentProgram.getSymbolTable().createLabel(address, name, namespace, sourceType);
        }
        catch (InvalidInputException e) {
            println("Could not label " + address + " as " + name + ": " + e.getMessage());
        }
    }
}
