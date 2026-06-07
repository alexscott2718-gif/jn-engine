// Dump the recovered RTTI class hierarchy for Neutron.exe.
//
// Run with:
//   analyzeHeadless ~/ghidra-projects JN_decomp -process Neutron.exe \
//     -noanalysis -readOnly -scriptPath tools/ghidra -postScript DumpHierarchy.java <out.md>
//
// @category JNScope

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Data;
import ghidra.program.model.listing.DataIterator;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;
import ghidra.program.model.mem.Memory;
import ghidra.program.model.mem.MemoryAccessException;
import ghidra.program.model.mem.MemoryBlock;
import ghidra.program.model.symbol.Namespace;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.Symbol;
import ghidra.program.model.symbol.SymbolIterator;
import ghidra.program.model.symbol.SymbolTable;

import java.io.File;
import java.io.FileWriter;
import java.io.PrintWriter;
import java.time.LocalDate;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;
import java.util.TreeSet;

public class DumpHierarchy extends GhidraScript {
    private static class ClassInfo {
        String name;
        List<String> parents = new ArrayList<>();
        List<String> children = new ArrayList<>();
        TreeSet<String> vftables = new TreeSet<>();
        TreeSet<String> constructors = new TreeSet<>();
        TreeSet<String> destructors = new TreeSet<>();
        int methodsInNamespace;
        int vtableSlots;
        Address typeDescriptor;
    }

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        String outPath = args.length > 0 ? args[0] : "/tmp/jn_hierarchy.md";

        Map<String, ClassInfo> classes = collectRttiClasses();
        collectSymbols(classes);
        collectFunctions(classes);
        collectVftableSlots(classes);
        buildChildren(classes);

        File outFile = new File(outPath);
        File parent = outFile.getParentFile();
        if (parent != null) {
            parent.mkdirs();
        }

        try (PrintWriter out = new PrintWriter(new FileWriter(outFile))) {
            writeMarkdown(out, classes);
        }

        println("Wrote " + outPath + " classes=" + classes.size() +
            " gameplay=" + countGameplay(classes) +
            " vftable_labeled=" + countWithVftables(classes));
    }

    private Map<String, ClassInfo> collectRttiClasses() throws Exception {
        Map<String, ClassInfo> classes = new TreeMap<>();
        Map<Address, String> typeDescriptors = collectTypeDescriptors();
        Map<Long, List<Address>> pointerIndex = collectPointerIndex();

        for (Map.Entry<Address, String> entry : typeDescriptors.entrySet()) {
            monitor.checkCancelled();
            Address typeDescriptor = entry.getKey();
            String className = entry.getValue();
            ClassInfo info = new ClassInfo();
            info.name = className;
            info.typeDescriptor = typeDescriptor;
            fillRttiRelationships(info, typeDescriptor, typeDescriptors, pointerIndex);
            classes.put(info.name, info);
        }

        return classes;
    }

    private Map<Address, String> collectTypeDescriptors() throws Exception {
        Map<Address, String> typeDescriptors = new HashMap<>();
        DataIterator data = currentProgram.getListing().getDefinedData(true);
        while (data.hasNext()) {
            monitor.checkCancelled();
            Data d = data.next();
            Object value = d.getValue();
            if (!(value instanceof String)) {
                continue;
            }

            String s = (String) value;
            int i = s.indexOf(".?AV");
            if (i < 0) {
                continue;
            }
            int j = s.indexOf("@@", i);
            if (j <= i) {
                continue;
            }

            String className = s.substring(i + 4, j);
            Address nameAddress = d.getAddress().add(i);
            Address[] candidates = new Address[] {
                safeSubtract(nameAddress, 8),
                d.getAddress()
            };
            for (Address candidate : candidates) {
                if (looksLikeTypeDescriptor(candidate, className)) {
                    typeDescriptors.put(candidate, className);
                    break;
                }
            }
        }
        return typeDescriptors;
    }

    private Map<Long, List<Address>> collectPointerIndex() throws Exception {
        Map<Long, List<Address>> pointerIndex = new HashMap<>();
        Memory memory = currentProgram.getMemory();
        for (MemoryBlock block : memory.getBlocks()) {
            monitor.checkCancelled();
            if (!block.isInitialized()) {
                continue;
            }

            long start = block.getStart().getOffset();
            long end = block.getEnd().getOffset();
            for (long off = start; off <= end - 3; off += 4) {
                Address address = toAddr(off);
                long value;
                try {
                    value = readU32(address);
                }
                catch (MemoryAccessException e) {
                    continue;
                }
                Address pointer = toProgramAddress(value);
                if (!isValidAddress(pointer)) {
                    continue;
                }
                pointerIndex.computeIfAbsent(value, k -> new ArrayList<>()).add(address);
            }
        }
        return pointerIndex;
    }

    private boolean looksLikeTypeDescriptor(Address address, String className) throws Exception {
        if (address == null || !isValidAddress(address) || !isValidAddress(address.add(8))) {
            return false;
        }

        Address typeInfoVftable = readPointer(address);
        if (typeInfoVftable == null || !isValidAddress(typeInfoVftable)) {
            return false;
        }
        if (readU32(address.add(4)) != 0) {
            return false;
        }

        String name = readCString(address.add(8), 256);
        return name.startsWith(".?AV" + className + "@@");
    }

    private void fillRttiRelationships(ClassInfo info, Address typeDescriptor,
            Map<Address, String> typeDescriptors, Map<Long, List<Address>> pointerIndex)
            throws Exception {
        List<Address> typeDescriptorPointers = pointerIndex.get(typeDescriptor.getOffset());
        if (typeDescriptorPointers == null) {
            return;
        }

        for (Address pointerAddress : typeDescriptorPointers) {
            monitor.checkCancelled();
            Address colAddress = safeSubtract(pointerAddress, 12);
            if (colAddress == null) {
                continue;
            }
            if (!looksLikeCompleteObjectLocator(colAddress, typeDescriptor)) {
                continue;
            }

            Address classHierarchy = readPointer(colAddress.add(16));
            if (classHierarchy == null || !isValidAddress(classHierarchy)) {
                continue;
            }

            int numBases = readS32(classHierarchy.add(8));
            if (numBases < 1 || numBases > 64) {
                continue;
            }

            Address baseArray = readPointer(classHierarchy.add(12));
            if (baseArray == null || !isValidAddress(baseArray)) {
                continue;
            }

            for (int i = 0; i < numBases; i++) {
                Address baseDescriptor = readPointer(baseArray.add(i * 4L));
                if (baseDescriptor == null || !isValidAddress(baseDescriptor)) {
                    continue;
                }
                Address baseTypeDescriptor = readPointer(baseDescriptor);
                String baseName = typeDescriptors.get(baseTypeDescriptor);
                if (baseName == null || baseName.equals(info.name) ||
                    info.parents.contains(baseName)) {
                    continue;
                }
                info.parents.add(baseName);
            }

            collectVftablesForCompleteObjectLocator(info, colAddress, pointerIndex);
        }
    }

    private boolean looksLikeCompleteObjectLocator(Address colAddress, Address expectedTypeDescriptor)
            throws Exception {
        if (!isValidAddress(colAddress) || !isValidAddress(colAddress.add(16))) {
            return false;
        }

        long signature = readU32(colAddress);
        long offset = readU32(colAddress.add(4));
        long cdOffset = readU32(colAddress.add(8));
        Address typeDescriptor = readPointer(colAddress.add(12));
        Address classHierarchy = readPointer(colAddress.add(16));

        if (signature > 1) {
            return false;
        }
        if (offset > 0x10000L && offset < 0xffff0000L) {
            return false;
        }
        if (cdOffset > 0x10000L && cdOffset < 0xffff0000L) {
            return false;
        }
        return expectedTypeDescriptor.equals(typeDescriptor) &&
            classHierarchy != null && isValidAddress(classHierarchy);
    }

    private void collectVftablesForCompleteObjectLocator(ClassInfo info, Address colAddress,
            Map<Long, List<Address>> pointerIndex) throws Exception {
        List<Address> colPointers = pointerIndex.get(colAddress.getOffset());
        if (colPointers == null) {
            return;
        }

        for (Address possibleBackPointer : colPointers) {
            monitor.checkCancelled();
            Address possibleVftable = possibleBackPointer.add(4);
            if (!isValidAddress(possibleVftable)) {
                continue;
            }
            Address firstSlot = readPointer(possibleVftable);
            if (firstSlot != null && isExecutableAddress(firstSlot)) {
                info.vftables.add(possibleVftable.toString());
            }
        }
    }

    private void collectSymbols(Map<String, ClassInfo> classes) {
        SymbolTable symbols = currentProgram.getSymbolTable();
        SymbolIterator it = symbols.getAllSymbols(true);
        while (it.hasNext()) {
            Symbol sym = it.next();
            String symName = sym.getName();
            if (!symName.toLowerCase().contains("vftable")) {
                continue;
            }

            String className = symbolClassName(sym, classes);
            if (className == null) {
                continue;
            }
            classes.get(className).vftables.add(sym.getAddress().toString());
        }
    }

    private String symbolClassName(Symbol sym, Map<String, ClassInfo> classes) {
        Namespace ns = sym.getParentNamespace();
        if (ns != null && classes.containsKey(ns.getName())) {
            return ns.getName();
        }

        String full = sym.getName(true);
        for (String className : classes.keySet()) {
            if (full.contains(className + "::") || full.contains("`" + className + "'")) {
                return className;
            }
        }
        return null;
    }

    private void collectFunctions(Map<String, ClassInfo> classes) {
        FunctionIterator it = currentProgram.getFunctionManager().getFunctions(true);
        while (it.hasNext()) {
            Function fn = it.next();
            Namespace ns = fn.getParentNamespace();
            if (ns == null || !classes.containsKey(ns.getName())) {
                continue;
            }

            ClassInfo info = classes.get(ns.getName());
            info.methodsInNamespace++;

            String fnName = fn.getName();
            String entry = fn.getEntryPoint().toString();
            if (fnName.equals(info.name) || fnName.equals(info.name + "_constructor") ||
                fnName.toLowerCase().contains("constructor")) {
                info.constructors.add(entry);
            }
            if (fnName.equals("~" + info.name) || fnName.toLowerCase().contains("destructor")) {
                info.destructors.add(entry);
            }
        }
    }

    private void collectVftableSlots(Map<String, ClassInfo> classes) throws Exception {
        for (ClassInfo info : classes.values()) {
            int totalSlots = 0;
            for (String vftable : info.vftables) {
                Address vftableAddress = toAddr(vftable);
                totalSlots += countVftableSlots(vftableAddress);
            }
            info.vtableSlots = totalSlots;
        }
    }

    private int countVftableSlots(Address vftableAddress) throws Exception {
        int count = 0;
        for (int i = 0; i < 256; i++) {
            Address slot = vftableAddress.add(i * 4L);
            if (!isValidAddress(slot)) {
                break;
            }
            Address target = readPointer(slot);
            if (target == null || !isExecutableAddress(target)) {
                break;
            }
            count++;
        }
        return count;
    }

    private void buildChildren(Map<String, ClassInfo> classes) {
        for (ClassInfo info : classes.values()) {
            if (info.parents.isEmpty()) {
                continue;
            }
            String directParent = info.parents.get(0);
            ClassInfo parentInfo = classes.get(directParent);
            if (parentInfo != null && !parentInfo.children.contains(info.name)) {
                parentInfo.children.add(info.name);
            }
        }
        for (ClassInfo info : classes.values()) {
            Collections.sort(info.children);
        }
    }

    private void writeMarkdown(PrintWriter out, Map<String, ClassInfo> classes) {
        out.println("# Neutron.exe RTTI Class Hierarchy");
        out.println();
        out.println("> Generated by `tools/ghidra/DumpHierarchy.java` from the saved " +
            "`~/ghidra-projects/JN_decomp` Ghidra project on " + LocalDate.now() + ".");
        out.println("> Source program: `" + currentProgram.getName() + "`.");
        out.println();
        out.println("## Summary");
        out.println();
        out.println("| Metric | Count |");
        out.println("|---|---:|");
        out.println("| MSVC RTTI TypeDescriptors | " + classes.size() + " |");
        out.println("| Gameplay `C*` classes, excluding `OMedia*` | " + countGameplay(classes) + " |");
        out.println("| Classes with vftable address(es) recovered | " + countWithVftables(classes) + " |");
        out.println();
        out.println("## Gameplay Classes");
        out.println();
        out.println("| Class | Base chain | Vftable(s) | Vtable slots | Methods in namespace | Ctor(s) | Dtor(s) | Children |");
        out.println("|---|---|---|---:|---:|---|---|---|");
        for (ClassInfo info : classes.values()) {
            if (!isGameplay(info.name)) {
                continue;
            }
            writeClassRow(out, info);
        }
        out.println();
        out.println("## Other RTTI Classes");
        out.println();
        out.println("| Class | Base chain | Vftable(s) | Vtable slots | Methods in namespace |");
        out.println("|---|---|---|---:|---:|");
        for (ClassInfo info : classes.values()) {
            if (isGameplay(info.name)) {
                continue;
            }
            out.printf("| `%s` | %s | %s | %d | %d |%n",
                info.name, formatBaseChain(info), formatList(info.vftables),
                info.vtableSlots, info.methodsInNamespace);
        }
    }

    private void writeClassRow(PrintWriter out, ClassInfo info) {
        out.printf("| `%s` | %s | %s | %d | %d | %s | %s | %s |%n",
            info.name,
            formatBaseChain(info),
            formatList(info.vftables),
            info.vtableSlots,
            info.methodsInNamespace,
            formatList(info.constructors),
            formatList(info.destructors),
            formatList(info.children));
    }

    private String formatBaseChain(ClassInfo info) {
        if (info.parents.isEmpty()) {
            return "-";
        }
        List<String> quoted = new ArrayList<>();
        for (String parent : info.parents) {
            quoted.add("`" + parent + "`");
        }
        return String.join(" -> ", quoted);
    }

    private String formatList(Iterable<String> values) {
        List<String> quoted = new ArrayList<>();
        for (String value : values) {
            quoted.add("`" + value + "`");
        }
        if (quoted.isEmpty()) {
            return "-";
        }
        return String.join(", ", quoted);
    }

    private int countGameplay(Map<String, ClassInfo> classes) {
        int count = 0;
        for (String name : classes.keySet()) {
            if (isGameplay(name)) {
                count++;
            }
        }
        return count;
    }

    private int countWithVftables(Map<String, ClassInfo> classes) {
        int count = 0;
        for (ClassInfo info : classes.values()) {
            if (!info.vftables.isEmpty()) {
                count++;
            }
        }
        return count;
    }

    private boolean isGameplay(String name) {
        return name.startsWith("C") && !name.startsWith("OMedia");
    }

    private Address safeSubtract(Address address, long value) {
        try {
            return address.subtract(value);
        }
        catch (Exception e) {
            return null;
        }
    }

    private boolean isValidAddress(Address address) {
        if (address == null) {
            return false;
        }
        Memory memory = currentProgram.getMemory();
        return memory.contains(address);
    }

    private boolean isExecutableAddress(Address address) {
        if (address == null) {
            return false;
        }
        MemoryBlock block = currentProgram.getMemory().getBlock(address);
        return block != null && block.isExecute();
    }

    private Address readPointer(Address address) throws MemoryAccessException {
        long value = readU32(address);
        if (value == 0) {
            return null;
        }
        Address pointer = toProgramAddress(value);
        return isValidAddress(pointer) ? pointer : pointer;
    }

    private Address toProgramAddress(long value) {
        return currentProgram.getAddressFactory()
            .getDefaultAddressSpace()
            .getAddress(value);
    }

    private int readS32(Address address) throws MemoryAccessException {
        return (int) readU32(address);
    }

    private long readU32(Address address) throws MemoryAccessException {
        byte[] bytes = new byte[4];
        currentProgram.getMemory().getBytes(address, bytes);
        return ((long) bytes[0] & 0xffL) |
            (((long) bytes[1] & 0xffL) << 8) |
            (((long) bytes[2] & 0xffL) << 16) |
            (((long) bytes[3] & 0xffL) << 24);
    }

    private String readCString(Address address, int maxLength) throws MemoryAccessException {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < maxLength; i++) {
            Address cur = address.add(i);
            if (!isValidAddress(cur)) {
                break;
            }
            int b = currentProgram.getMemory().getByte(cur) & 0xff;
            if (b == 0) {
                break;
            }
            sb.append((char) b);
        }
        return sb.toString();
    }
}
