# CalcForge v3.0

**A powerful, intelligent calculator application with advanced mathematical capabilities, date arithmetic, timecode operations, and multi-sheet support.**

Or, as I like to say, if a spreadsheet and scientific calculator had a baby.... then the baby mutated an extra arm.

Long and short of this app is this: I use my calculator on my windows pc all the time, but the default calc is limited in terms of history and more advanced functionality. I also do conversions a lot and work with timecode/frames in the film/video/gaming/animation world. There are various "supercalcs" out there, but none of them really worked the way I wanted a calculator to work, or they were incredibly buggy. So, I decided to make this a fun project to brush off my dormant comp sci chops and give cursor/claude combo a try to see if I could make my own app that functioned exactly like I wanted it. Hence, CalcForge was born! Below is a list of features, feel free to download it and give it a whirl, suggest improvements, all that jazz. On the windows side, i bind a keyboard shortcut to launch the app via power toys and it works like a champ. 

![CalcForge](https://img.shields.io/badge/version-3.0-blue.svg)
![Python](https://img.shields.io/badge/python-3.8+-green.svg)
![License](https://img.shields.io/badge/license-GPL--3.0-red.svg)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20macOS%20%7C%20Linux-lightgrey.svg)

## 🚀 Features

### 📊 **Smart Calculator Interface**
- **Dual-pane design**: Formula editor with live results panel
- **Multi-sheet support**: Create and manage multiple calculation worksheets
- **Line references**: Reference previous calculations with `LN1`, `LN2`, etc.
- **Cross-sheet references**: Reference calculations from other sheets with `S.SheetName.LN2`
- **Syntax highlighting**: Color-coded formulas with intelligent highlighting
- **Auto-completion**: Smart suggestions for functions and commands
- **Real-time evaluation**: See results instantly as you type

### 🧮 **Mathematical Operations**

#### Basic Arithmetic
- Standard operations: `+`, `-`, `*`, `/`, `^`
- Parentheses grouping: `(2 + 3) * 4`
- Number formatting with thousands separators

#### Advanced Mathematics
- **Trigonometric**: `sin()`, `cos()`, `tan()`, `asin()`, `acos()`, `atan()`
- **Hyperbolic**: `sinh()`, `cosh()`, `tanh()`, `asinh()`, `acosh()`, `atanh()`
- **Logarithmic**: `log()`, `log10()`, `log2()`, `exp()`
- **Power functions**: `sqrt()`, `pow(x,y)`
- **Rounding**: `ceil()`, `floor()`, `abs()`, `truncate(value, decimals)`
- **Number theory**: `factorial()`, `gcd()`, `lcm()`
- **Constants**: `pi`, `e`
- **Angle conversion**: `degrees()`, `radians()`

### 📈 **Statistical Analysis**

#### Basic Statistics
- `sum(start-end)` - Sum of values
- `mean(start-end)` - Average
- `median(start-end)` - Middle value
- `mode(start-end)` - Most frequent value
- `min(start-end)` - Minimum value
- `max(start-end)` - Maximum value
- `range(start-end)` - Max - Min
- `count(start-end)` - Count of values

#### Advanced Statistics
- `variance(start-end)` - Statistical variance
- `stdev(start-end)` - Standard deviation
- `geomean(start-end)` - Geometric mean
- `harmmean(start-end)` - Harmonic mean
- `product(start-end)` - Product of values
- `sumsq(start-end)` - Sum of squares
- `perc5(start-end)` - 5th percentile
- `perc95(start-end)` - 95th percentile

### 📅 **Date & Time Operations**

#### Date Formats
- **Numeric**: `D.05.09.2030`, `D05092030`, `D.592030`
- **Named months**: `D.March 5, 1976`, `DMarch 5, 1976`
- **Various separators**: `/`, `.`, spaces

#### Date Arithmetic
- **Add/subtract days**: `D.03.05.1976 + 100`
- **Date ranges**: `D.03.05.1976 - D.04.15.1976`
- **Business days**: `D.03.05.1976 W+ 100` (skips weekends)
- **Business day counting**: `D.03.05.1976 W- D.04.15.1976`

### 🎬 **Timecode Operations**
- **Professional timecode support**: `TC(fps, timecode)`
- **Multiple frame rates**: 23.976, 29.97 DF, 59.94 DF, and more
- **Timecode arithmetic**: Add, subtract, and convert timecodes
- **Frame counting**: Convert between timecode and frame numbers

### 🔄 **Unit Conversions**
- **Length**: miles, kilometers, feet, meters, inches, yards
- **Weight**: pounds, kilograms, grams, ounces, tons
- **Volume**: gallons, liters, quarts, pints, cups, milliliters
- **Simple syntax**: `1 mile to km`, `5 pounds to kg`

### 💻 **User Interface Features**

#### Editor Enhancements
- **Line numbers**: Clear line identification
- **Syntax highlighting**: Color-coded formulas and references
- **Smart selection**: Ctrl+Up for parentheses, Ctrl+Down for lines
- **Number navigation**: Ctrl+Left/Right to jump between numbers
- **Font scaling**: Ctrl+Mouse wheel to zoom
- **Auto-correction**: Automatic capitalization of references

#### Visual Feedback
- **Live highlighting**: Referenced lines are highlighted in real-time
- **Expression tooltips**: Hover over operators to see sub-results
- **Error indicators**: Clear error messages for invalid expressions
- **Separator lines**: Visual separation for statistical functions

#### Keyboard Shortcuts
- **Ctrl+C**: Copy result from current line
- **Alt+C**: Copy line content
- **Ctrl+Left/Right**: Navigate between numbers
- **Ctrl+Up**: Expand selection with parentheses
- **Ctrl+Down**: Select entire line

### 📝 **Comments & Documentation**
- **Comment lines**: Start with `:::` for documentation
- **Persistent storage**: Worksheets automatically saved and restored
- **Multiple tabs**: Organize work across different sheets

## 🛠 Installation

### Prerequisites
- Python 3.8 or higher
- PySide6
- pint (for unit conversions)

### Install Dependencies
```bash
pip install PySide6 pint
```

### Run CalcForge
```bash
python CalcForge.3.0.py
```

## 📖 Usage Examples

### Basic Calculations
```
100 + 50 * 2
sqrt(144)
pi * 2^2
```

### Line References
```
100
LN1 * 2
LN1 + LN2
```

### Cross-Sheet References
```
LN1 + S.Budget.LN5
S.Data.LN2 * 1.08
```

### Date Operations
```
D.March 15, 2024
D.03.15.2024 + 30
D.01.01.2024 W+ 100
D.12.31.2024 - D.01.01.2024
```

### Statistical Analysis
```
100
200
150
175
sum(1-4)
mean(1-4)
stdev(1-4)
```

### Unit Conversions
```
1 mile to km
5 pounds to kg
2 gallons to liters
```

### Timecode Operations
```
TC(29.97, "01:00:00:00")
TC(24, "00:30:15:12" + "00:15:30:06")
```

## 🎯 Advanced Features

### Multi-Sheet Workflows
- Create multiple calculation sheets
- Reference data between sheets
- Rename sheets for organization
- Drag to reorder tabs

### Professional Timecode
- Support for drop-frame and non-drop-frame rates
- Industry-standard frame rates (23.976, 29.97, 59.94)
- Timecode arithmetic and conversion

### Statistical Analysis
- Comprehensive statistical functions
- Range-based calculations
- Percentile analysis
- Visual separation of statistical blocks

## 🔧 Technical Details

### Built With
- **Python 3.8+**: Core language
- **PySide6**: Modern Qt-based GUI framework
- **Pint**: Unit conversion library
- **Statistics**: Built-in statistical functions

### Architecture
- **Multi-threaded evaluation**: Real-time calculation updates
- **Persistent storage**: JSON-based worksheet saving
- **Cross-platform**: Windows, macOS, and Linux support
- **Modular design**: Extensible function system

## 🤝 Contributing

CalcForge is open source software licensed under GPL 3.0. Contributions are welcome!

### Development Setup
1. Clone the repository
2. Install dependencies: `pip install PySide6 pint`
3. Run the application: `python CalcForge.3.0.py`

### License
This project is licensed under the GNU General Public License v3.0 - see the [LICENSE](LICENSE) file for details.

## 📞 Support

- **Issues**: Report bugs and request features via GitHub Issues
- **Documentation**: Built-in help system (click the `?` button)
- **Community**: Discussions and questions welcome

## 🎉 Acknowledgments

CalcForge combines the power of a scientific calculator with the flexibility of a spreadsheet, designed for professionals who need advanced mathematical capabilities in an intuitive interface.

---

**CalcForge v3.0** - Where calculation meets innovation. 🚀
