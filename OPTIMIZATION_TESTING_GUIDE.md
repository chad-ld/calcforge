# Tab Switching Optimization Testing Guide

## 🚀 How to Test the 3-Stage Optimization System

With debug output enabled, you can now verify that all optimization stages are working correctly. Here's how to test each stage:

---

## **Stage 1 Testing: Basic Change Tracking**

### ✅ **Test: No Changes = Instant Tab Switching**

1. **Setup**: Open CalcForge with multiple sheets
2. **Action**: Switch between tabs WITHOUT making any changes
3. **Expected Output**: 
   ```
   Reason: No changes detected - SKIPPING EVALUATION ✅ [STAGE 1 OPTIMIZATION]
   ```
4. **Result**: Tab switching should be near-instant (~10ms)

### ✅ **Test: Modified Sheet = Full Evaluation**

1. **Setup**: Add some content to a sheet (e.g., `100 + 200`)
2. **Action**: Switch away from that sheet, then back to it
3. **Expected Output**:
   ```
   📊 [FULL EVAL] Complete sheet evaluation
   Reason: Current sheet X was modified
   ```
4. **Result**: Full evaluation happens, but only when necessary

---

## **Stage 2 Testing: Selective Cross-Sheet Evaluation**

### ✅ **Test: Cross-Sheet Reference Detection**

1. **Setup**: Create cross-sheet references:
   - **Sheet 1**: `S.Data.LN1 + S.Data.LN2`
   - **Sheet 2 (named "Data")**: `100` and `200`

2. **Action**: 
   - Modify something in "Data" sheet
   - Switch to Sheet 1

3. **Expected Output**:
   ```
   ⚡ [STAGE 2] Cross-sheet selective evaluation
   Reason: Previous sheet X changed + current has cross-refs
   ```

4. **Result**: Only cross-sheet reference lines are re-evaluated (faster than full eval)

---

## **Stage 3 Testing: Dependency Graph Optimization**

### ✅ **Test: Dependency Graph Building**

1. **Setup**: Create a dependency chain:
   - **Sheet "Source"**: `1000`, `2000`
   - **Sheet "Calc"**: `S.Source.LN1 * 2`
   - **Sheet "Final"**: `S.Calc.LN1 + S.Source.LN2`

2. **Action**: Wait for initial load or make any text change

3. **Expected Output**:
   ```
   🔄 [STAGE 3] Building dependency graph...
   📊 Dependency Graph Summary:
     Calc (#1):
       Depends on: ['Source']
     Final (#2):
       Depends on: ['Calc', 'Source']
     Source (#0):
       Referenced by: ['Calc', 'Final']
   ✅ Dependency graph complete
   ```

### ✅ **Test: Smart Dependency Updates**

1. **Setup**: Use the dependency chain from above
2. **Action**: 
   - Modify "Source" sheet (change `1000` to `1500`)
   - Switch to "Final" sheet

3. **Expected Output**:
   ```
   🚀 [STAGE 3] Dependency-aware selective evaluation
   Reason: Sheet 2 depends on changed sheets
   ```

4. **Result**: Only evaluates cross-sheet lines, but knows exactly which sheets need updating

### ✅ **Test: Batch Processing**

1. **Setup**: Same dependency chain
2. **Action**: Make multiple rapid changes to "Source" sheet
3. **Expected Output**:
   ```
   ⏱️  [STAGE 3] Scheduling dependency updates for sheets: {1, 2}
   🔄 [STAGE 3] Processing batch updates for sheets: {1, 2}
   ✅ [STAGE 3] Batch updates complete
   ```

---

## **Performance Comparison Test**

### 🔧 **Before vs After Test**

1. **Disable optimization**: Set `DEBUG_TAB_SWITCHING = False` and comment out all optimization logic
2. **Enable optimization**: Restore the optimization code
3. **Compare**: Switch between sheets with cross-sheet references

**Expected Results:**
- **Without optimization**: ~500ms delay every tab switch
- **With Stage 1**: ~10ms for unchanged sheets, ~200ms when needed  
- **With Stage 2**: ~10ms for unchanged, ~100ms for cross-sheet updates
- **With Stage 3**: ~10ms for unchanged, ~50ms for dependency updates

---

## **Debug Output Legend**

| Symbol | Meaning |
|--------|---------|
| ✅ | Stage 1: Skipping evaluation (instant) |
| ⚡ | Stage 2: Selective cross-sheet evaluation |
| 🚀 | Stage 3: Dependency-aware evaluation |
| 📊 | Full evaluation (when actually needed) |
| 🔄 | Building dependency graph |
| ⏱️  | Scheduling batch updates |

---

## **Quick Test Scenarios**

### **Scenario A: Independent Sheets**
- Create sheets with no cross-references
- **Expected**: All tab switches show "SKIPPING EVALUATION ✅"

### **Scenario B: Simple Cross-Sheet**
- Sheet A: `100`
- Sheet B: `S.A.LN1 + 50`
- **Expected**: Changing A triggers selective evaluation in B

### **Scenario C: Complex Dependencies**
- Sheet A: `10`
- Sheet B: `S.A.LN1 * 2` 
- Sheet C: `S.B.LN1 + S.A.LN1`
- **Expected**: Changing A triggers dependency updates for both B and C

### **Scenario D: No Dependencies**
- Multiple sheets, no cross-references
- **Expected**: Perfect Stage 1 optimization - all instant switches

---

## **Troubleshooting**

**If you don't see optimization working:**

1. ✅ Check debug output is enabled (`DEBUG_TAB_SWITCHING = True`)
2. ✅ Verify cross-sheet references use correct format: `S.SheetName.LN1`
3. ✅ Ensure sheet names match exactly (case-insensitive)
4. ✅ Check that sheets have the `has_cross_sheet_refs` flag set correctly
5. ✅ Look for dependency graph output after any content changes

**Performance not as expected:**
- Ensure you're testing realistic scenarios with meaningful content
- Check that evaluation times in debug output match expectations
- Verify that "SKIPPING EVALUATION" appears for unchanged sheets 