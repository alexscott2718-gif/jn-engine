// Dump one gameplay class's vtable slots, owned decompilation, and candidate
// this-offset accesses for per-class spec writing.
// @category JNScope
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileOptions;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.mem.MemoryBlock;
import ghidra.program.model.symbol.Namespace;

import java.io.PrintWriter;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.TreeMap;
import java.util.TreeSet;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class DumpClass extends GhidraScript {
    private static final String DEFAULT_HIERARCHY =
        "/home/scotty/jn-engine/docs/decomp/_hierarchy.md";
    private static final Pattern FIELD_ARROW =
        Pattern.compile("this->field_0x([0-9a-fA-F]+)");
    private static final Pattern THIS_PLUS =
        Pattern.compile("this\\s*\\+\\s*0x([0-9a-fA-F]+)");

    private static class ClassInfo {
        String name;
        String baseChain;
        ArrayList<Address> vftables = new ArrayList<>();
    }

    private static class SlotInfo {
        int vtableIndex;
        int slot;
        Address target;
        Function function;
        String owner;
    }

    private DecompInterface decompiler;

    @Override
    public void run() throws Exception {
        if (getScriptArgs().length < 1) {
            println("usage: DumpClass.java <ClassName|vftableAddr> [outPath] [hierarchyPath]");
            return;
        }

        String target = getScriptArgs()[0];
        String hierarchyPath = getScriptArgs().length >= 3 ? getScriptArgs()[2] : DEFAULT_HIERARCHY;
        Map<String, ClassInfo> classes = parseHierarchy(hierarchyPath);
        ClassInfo info = resolveTarget(classes, target);
        if (info == null) {
            println("No class found for " + target);
            return;
        }

        String outPath = getScriptArgs().length >= 2
            ? getScriptArgs()[1]
            : "/tmp/decomp_" + info.name + ".md";

        decompiler = new DecompInterface();
        decompiler.setOptions(new DecompileOptions());
        decompiler.openProgram(currentProgram);

        ArrayList<SlotInfo> slots = collectSlots(info);
        LinkedHashMap<Address, Function> owned = collectOwnedFunctions(info.name, slots);
        TreeMap<Long, TreeSet<String>> offsets = new TreeMap<>();
        LinkedHashMap<Address, String> decompiled = new LinkedHashMap<>();
        for (Map.Entry<Address, Function> entry : owned.entrySet()) {
            monitor.checkCancelled();
            String code = decompile(entry.getValue());
            decompiled.put(entry.getKey(), code);
            collectOffsets(offsets, entry.getValue(), code);
        }

        try (PrintWriter out = new PrintWriter(outPath)) {
            writeReport(out, info, slots, owned, offsets, decompiled);
        }
        println("Wrote " + outPath + " slots=" + slots.size() +
            " owned_methods=" + owned.size() + " offsets=" + offsets.size());
    }

    private Map<String, ClassInfo> parseHierarchy(String path) throws Exception {
        Map<String, ClassInfo> classes = new TreeMap<>();
        boolean inTable = false;
        for (String line : Files.readAllLines(Paths.get(path))) {
            if (line.startsWith("## Gameplay Classes")) {
                inTable = true;
                continue;
            }
            if (inTable && line.startsWith("## ")) {
                break;
            }
            if (!inTable || !line.startsWith("| `")) {
                continue;
            }
            String[] parts = line.substring(1, line.length() - 1).split("\\|");
            if (parts.length < 8) {
                continue;
            }
            ClassInfo info = new ClassInfo();
            info.name = clean(parts[0]);
            info.baseChain = clean(parts[1]);
            String vftableCell = clean(parts[2]);
            if (info.name.equals("Class")) {
                continue;
            }
            if (!vftableCell.isEmpty()) {
                for (String token : vftableCell.split(",")) {
                    token = token.trim();
                    if (!token.isEmpty()) {
                        info.vftables.add(toAddr(Long.parseLong(token, 16)));
                    }
                }
            }
            classes.put(info.name, info);
        }
        return classes;
    }

    private String clean(String cell) {
        cell = cell.trim().replace("`", "");
        return cell.equals("-") ? "" : cell;
    }

    private ClassInfo resolveTarget(Map<String, ClassInfo> classes, String target) {
        ClassInfo direct = classes.get(target);
        if (direct != null) {
            return direct;
        }
        String normalized = target.toLowerCase();
        if (normalized.startsWith("0x")) {
            normalized = normalized.substring(2);
        }
        for (ClassInfo info : classes.values()) {
            for (Address vftable : info.vftables) {
                if (vftable.toString().equalsIgnoreCase(normalized)) {
                    return info;
                }
            }
        }
        return null;
    }

    private ArrayList<SlotInfo> collectSlots(ClassInfo info) throws Exception {
        ArrayList<SlotInfo> slots = new ArrayList<>();
        for (int vti = 0; vti < info.vftables.size(); vti++) {
            Address vftable = info.vftables.get(vti);
            for (int slot = 0; slot < 512; slot++) {
                monitor.checkCancelled();
                Address target = readPointer(vftable.add(slot * 4L));
                if (target == null || !isExecutableAddress(target)) {
                    break;
                }
                Function fn = getFunctionContaining(target);
                SlotInfo si = new SlotInfo();
                si.vtableIndex = vti;
                si.slot = slot;
                si.target = target;
                si.function = fn;
                si.owner = ownerName(fn);
                slots.add(si);
            }
        }
        return slots;
    }

    private LinkedHashMap<Address, Function> collectOwnedFunctions(String className,
            ArrayList<SlotInfo> slots) {
        LinkedHashMap<Address, Function> owned = new LinkedHashMap<>();
        for (SlotInfo slot : slots) {
            if (slot.function == null || !className.equals(slot.owner)) {
                continue;
            }
            owned.putIfAbsent(slot.function.getEntryPoint(), slot.function);
        }
        return owned;
    }

    private String ownerName(Function fn) {
        if (fn == null) {
            return "";
        }
        Namespace ns = fn.getParentNamespace();
        return ns == null ? "" : ns.getName();
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

    private String decompile(Function fn) {
        DecompileResults res = decompiler.decompileFunction(fn, 90, monitor);
        if (res != null && res.getDecompiledFunction() != null) {
            return res.getDecompiledFunction().getC();
        }
        return "(decompile failed)";
    }

    private void collectOffsets(TreeMap<Long, TreeSet<String>> offsets, Function fn, String code) {
        for (String line : code.split("\\R")) {
            if (!line.contains("this")) {
                continue;
            }
            collectOffsetPattern(offsets, fn, line, FIELD_ARROW);
            collectOffsetPattern(offsets, fn, line, THIS_PLUS);
        }
    }

    private void collectOffsetPattern(TreeMap<Long, TreeSet<String>> offsets, Function fn,
            String line, Pattern pattern) {
        Matcher matcher = pattern.matcher(line);
        while (matcher.find()) {
            long offset = Long.parseLong(matcher.group(1), 16);
            offsets.computeIfAbsent(offset, k -> new TreeSet<>())
                .add(fn.getName() + "@" + fn.getEntryPoint());
        }
    }

    private void writeReport(PrintWriter out, ClassInfo info, ArrayList<SlotInfo> slots,
            LinkedHashMap<Address, Function> owned, TreeMap<Long, TreeSet<String>> offsets,
            LinkedHashMap<Address, String> decompiled) {
        out.println("# DumpClass: " + info.name);
        out.println();
        out.println("| Item | Value |");
        out.println("|---|---|");
        out.println("| Class | `" + info.name + "` |");
        out.println("| Base chain | `" + info.baseChain + "` |");
        out.println("| Vftables | `" + formatVftables(info) + "` |");
        out.println("| Slots walked | " + slots.size() + " |");
        out.println("| Owned methods decompiled | " + owned.size() + " |");
        out.println();

        out.println("## Vtable Slots");
        out.println();
        out.println("| Vtable | Slot | Target | Function | Owner |");
        out.println("|---:|---:|---|---|---|");
        for (SlotInfo slot : slots) {
            Function fn = slot.function;
            String fname = fn == null ? "" : fn.getName();
            String faddr = fn == null ? "" : " @ " + fn.getEntryPoint();
            out.println("| " + slot.vtableIndex + " | " + slot.slot + " | `" + slot.target +
                "` | `" + fname + faddr + "` | `" + slot.owner + "` |");
        }
        out.println();

        out.println("## Candidate This Offsets");
        out.println();
        out.println("| Offset | References | Functions |");
        out.println("|---:|---:|---|");
        for (Map.Entry<Long, TreeSet<String>> entry : offsets.entrySet()) {
            out.println("| `0x" + Long.toHexString(entry.getKey()) + "` | " +
                entry.getValue().size() + " | `" + String.join("`, `", entry.getValue()) + "` |");
        }
        out.println();

        out.println("## Owned Method Decompilation");
        for (Map.Entry<Address, String> entry : decompiled.entrySet()) {
            Function fn = getFunctionAt(entry.getKey());
            out.println();
            out.println("### `" + (fn == null ? "" : fn.getName()) + "` @ `" + entry.getKey() + "`");
            out.println();
            out.println("```c");
            out.println(entry.getValue());
            out.println("```");
        }
    }

    private String formatVftables(ClassInfo info) {
        ArrayList<String> addrs = new ArrayList<>();
        for (Address address : info.vftables) {
            addrs.add(address.toString());
        }
        return String.join(", ", addrs);
    }
}
