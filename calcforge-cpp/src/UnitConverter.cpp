#include "UnitConverter.h"
#include "Logger.h"
#include "CalcForgeResult.h"
#include "CalcForgeException.h"
#include "RegexUtils.h"
#include <QDebug>
#include <QRegularExpression>

UnitConverter::UnitConverter()
{
    initializeUnits();
}

UnitConversionResult UnitConverter::convertExpression(const QString &expression)
{
    // Pattern to match unit conversions like "5 feet to meters"
    QRegularExpression pattern(R"(^([\d.]+)\s+(.+?)\s+to\s+(.+?)$)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = pattern.match(expression.trimmed());

    if (match.hasMatch()) {
        bool ok;
        double value = match.captured(1).toDouble(&ok);
        if (!ok) {
            LOG_DEBUG(QString("Invalid numeric value in unit conversion: %1").arg(match.captured(1)));
            return makeUnitError(QString("Error: Invalid number '%1'").arg(match.captured(1)));
        }

        QString fromUnit = match.captured(2).trimmed().toLower();
        QString toUnit = match.captured(3).trimmed().toLower();

        return convert(value, fromUnit, toUnit);
    }

    return makeUnitError("Invalid conversion expression format");
}

UnitConversionResult UnitConverter::convert(double value, const QString &fromUnit, const QString &toUnit)
{
    // Normalize unit names to standard abbreviations
    QString fromAbbr = normalizeUnit(fromUnit.toLower());
    QString toAbbr = normalizeUnit(toUnit.toLower());

    if (fromAbbr.isEmpty()) {
        LOG_DEBUG(QString("Unknown from unit in conversion: %1").arg(fromUnit));
        return makeUnitError(QString("Error: Unknown unit '%1'").arg(fromUnit));
    }

    if (toAbbr.isEmpty()) {
        LOG_DEBUG(QString("Unknown to unit in conversion: %1").arg(toUnit));
        return makeUnitError(QString("Error: Unknown unit '%1'").arg(toUnit));
    }

    // Check if units are in the same category
    QString fromCategory = getUnitCategory(fromAbbr);
    QString toCategory = getUnitCategory(toAbbr);

    if (fromCategory.isEmpty() || toCategory.isEmpty() || fromCategory != toCategory) {
        LOG_DEBUG(QString("Units not in same category: %1 (%2) to %3 (%4)")
                  .arg(fromUnit).arg(fromCategory).arg(toUnit).arg(toCategory));
        return makeUnitError(QString("Error: Cannot convert %1 to %2 (different unit types)").arg(fromUnit).arg(toUnit));
    }

    double convertedValue;

    // Handle temperature conversions specially
    if (fromCategory == "temperature") {
        convertedValue = convertTemperature(value, fromAbbr, toAbbr);
    } else {
        convertedValue = convertWithinCategory(value, fromAbbr, toAbbr, fromCategory);
    }

    if (std::isnan(convertedValue)) {
        return makeUnitError(QString("Error: Conversion failed for %1 to %2").arg(fromUnit).arg(toUnit));
    }

    // Get display name for result
    QString displayName = getDisplayName(toAbbr);

    return UnitConversionResult::success(std::make_pair(convertedValue, displayName));
}

bool UnitConverter::isValidUnit(const QString &unit) const
{
    return !normalizeUnit(unit.toLower()).isEmpty();
}

QString UnitConverter::getDisplayName(const QString &unitAbbr) const
{
    return m_displayNames.value(unitAbbr, unitAbbr);
}

QString UnitConverter::getAbbreviation(const QString &unitName) const
{
    return m_unitAbbreviations.value(unitName.toLower(), QString());
}

void UnitConverter::initializeUnits()
{
    initializeDistanceUnits();
    initializeAreaUnits();
    initializeWeightUnits();
    initializeVolumeUnits();
    initializeTemperatureUnits();
    initializeTimeUnits();
    initializePowerUnits();
    initializeEnergyUnits();
    initializeVelocityUnits();
    initializePressureUnits();
}

void UnitConverter::initializeDistanceUnits()
{
    // Distance unit abbreviations (name -> abbreviation)
    m_unitAbbreviations["meter"] = "m";
    m_unitAbbreviations["meters"] = "m";
    m_unitAbbreviations["m"] = "m";
    m_unitAbbreviations["kilometer"] = "km";
    m_unitAbbreviations["kilometers"] = "km";
    m_unitAbbreviations["km"] = "km";
    m_unitAbbreviations["mile"] = "mi";
    m_unitAbbreviations["miles"] = "mi";
    m_unitAbbreviations["mi"] = "mi";
    m_unitAbbreviations["yard"] = "yd";
    m_unitAbbreviations["yards"] = "yd";
    m_unitAbbreviations["yd"] = "yd";
    m_unitAbbreviations["foot"] = "ft";
    m_unitAbbreviations["feet"] = "ft";
    m_unitAbbreviations["ft"] = "ft";
    m_unitAbbreviations["inch"] = "in";
    m_unitAbbreviations["inches"] = "in";
    m_unitAbbreviations["in"] = "in";
    m_unitAbbreviations["centimeter"] = "cm";
    m_unitAbbreviations["centimeters"] = "cm";
    m_unitAbbreviations["cm"] = "cm";
    m_unitAbbreviations["millimeter"] = "mm";
    m_unitAbbreviations["millimeters"] = "mm";
    m_unitAbbreviations["mm"] = "mm";
    
    // Display names (abbreviation -> display name)
    m_displayNames["m"] = "meters";
    m_displayNames["km"] = "kilometers";
    m_displayNames["mi"] = "miles";
    m_displayNames["yd"] = "yards";
    m_displayNames["ft"] = "feet";
    m_displayNames["in"] = "inches";
    m_displayNames["cm"] = "centimeters";
    m_displayNames["mm"] = "millimeters";
    
    // Categories
    m_unitCategories["m"] = "distance";
    m_unitCategories["km"] = "distance";
    m_unitCategories["mi"] = "distance";
    m_unitCategories["yd"] = "distance";
    m_unitCategories["ft"] = "distance";
    m_unitCategories["in"] = "distance";
    m_unitCategories["cm"] = "distance";
    m_unitCategories["mm"] = "distance";
    
    // Conversion factors to meters
    m_distanceFactors["m"] = 1.0;
    m_distanceFactors["km"] = 1000.0;
    m_distanceFactors["mi"] = 1609.344;
    m_distanceFactors["yd"] = 0.9144;
    m_distanceFactors["ft"] = 0.3048;
    m_distanceFactors["in"] = 0.0254;
    m_distanceFactors["cm"] = 0.01;
    m_distanceFactors["mm"] = 0.001;
}

void UnitConverter::initializeAreaUnits()
{
    // Area unit abbreviations
    m_unitAbbreviations["square meter"] = "m²";
    m_unitAbbreviations["square meters"] = "m²";
    m_unitAbbreviations["sq m"] = "m²";
    m_unitAbbreviations["m²"] = "m²";
    m_unitAbbreviations["m^2"] = "m²";

    m_unitAbbreviations["square kilometer"] = "km²";
    m_unitAbbreviations["square kilometers"] = "km²";
    m_unitAbbreviations["sq km"] = "km²";
    m_unitAbbreviations["km²"] = "km²";
    m_unitAbbreviations["km^2"] = "km²";

    m_unitAbbreviations["square foot"] = "ft²";
    m_unitAbbreviations["square feet"] = "ft²";
    m_unitAbbreviations["sq ft"] = "ft²";
    m_unitAbbreviations["ft²"] = "ft²";
    m_unitAbbreviations["ft^2"] = "ft²";

    m_unitAbbreviations["square inch"] = "in²";
    m_unitAbbreviations["square inches"] = "in²";
    m_unitAbbreviations["sq in"] = "in²";
    m_unitAbbreviations["in²"] = "in²";
    m_unitAbbreviations["in^2"] = "in²";

    m_unitAbbreviations["square yard"] = "yd²";
    m_unitAbbreviations["square yards"] = "yd²";
    m_unitAbbreviations["sq yd"] = "yd²";
    m_unitAbbreviations["yd²"] = "yd²";
    m_unitAbbreviations["yd^2"] = "yd²";

    m_unitAbbreviations["square mile"] = "mi²";
    m_unitAbbreviations["square miles"] = "mi²";
    m_unitAbbreviations["sq mi"] = "mi²";
    m_unitAbbreviations["mi²"] = "mi²";
    m_unitAbbreviations["mi^2"] = "mi²";

    m_unitAbbreviations["acre"] = "acre";
    m_unitAbbreviations["acres"] = "acre";

    m_unitAbbreviations["hectare"] = "ha";
    m_unitAbbreviations["hectares"] = "ha";
    m_unitAbbreviations["ha"] = "ha";

    // Display names
    m_displayNames["m²"] = "square meters";
    m_displayNames["km²"] = "square kilometers";
    m_displayNames["ft²"] = "square feet";
    m_displayNames["in²"] = "square inches";
    m_displayNames["yd²"] = "square yards";
    m_displayNames["mi²"] = "square miles";
    m_displayNames["acre"] = "acres";
    m_displayNames["ha"] = "hectares";

    // Categories
    m_unitCategories["m²"] = "area";
    m_unitCategories["km²"] = "area";
    m_unitCategories["ft²"] = "area";
    m_unitCategories["in²"] = "area";
    m_unitCategories["yd²"] = "area";
    m_unitCategories["mi²"] = "area";
    m_unitCategories["acre"] = "area";
    m_unitCategories["ha"] = "area";

    // Conversion factors to square meters
    m_areaFactors["m²"] = 1.0;
    m_areaFactors["km²"] = 1000000.0;  // 1 km² = 1,000,000 m²
    m_areaFactors["ft²"] = 0.092903;   // 1 ft² = 0.092903 m²
    m_areaFactors["in²"] = 0.00064516; // 1 in² = 0.00064516 m²
    m_areaFactors["yd²"] = 0.836127;   // 1 yd² = 0.836127 m²
    m_areaFactors["mi²"] = 2589988.11; // 1 mi² = 2,589,988.11 m²
    m_areaFactors["acre"] = 4046.86;   // 1 acre = 4,046.86 m²
    m_areaFactors["ha"] = 10000.0;     // 1 hectare = 10,000 m²
}

void UnitConverter::initializeWeightUnits()
{
    // Weight unit abbreviations
    m_unitAbbreviations["pound"] = "lb";
    m_unitAbbreviations["pounds"] = "lb";
    m_unitAbbreviations["lb"] = "lb";
    m_unitAbbreviations["lbs"] = "lb";
    m_unitAbbreviations["kilogram"] = "kg";
    m_unitAbbreviations["kilograms"] = "kg";
    m_unitAbbreviations["kg"] = "kg";
    m_unitAbbreviations["gram"] = "g";
    m_unitAbbreviations["grams"] = "g";
    m_unitAbbreviations["g"] = "g";
    m_unitAbbreviations["ounce"] = "oz";
    m_unitAbbreviations["ounces"] = "oz";
    m_unitAbbreviations["oz"] = "oz";
    m_unitAbbreviations["ton"] = "t";
    m_unitAbbreviations["tons"] = "t";
    m_unitAbbreviations["t"] = "t";
    
    // Display names
    m_displayNames["lb"] = "pounds";
    m_displayNames["kg"] = "kilograms";
    m_displayNames["g"] = "grams";
    m_displayNames["oz"] = "ounces";
    m_displayNames["t"] = "tons";
    
    // Categories
    m_unitCategories["lb"] = "weight";
    m_unitCategories["kg"] = "weight";
    m_unitCategories["g"] = "weight";
    m_unitCategories["oz"] = "weight";
    m_unitCategories["t"] = "weight";
    
    // Conversion factors to kilograms
    m_weightFactors["kg"] = 1.0;
    m_weightFactors["lb"] = 0.453592;
    m_weightFactors["g"] = 0.001;
    m_weightFactors["oz"] = 0.0283495;
    m_weightFactors["t"] = 1000.0;
}

void UnitConverter::initializeVolumeUnits()
{
    // Volume unit abbreviations
    m_unitAbbreviations["liter"] = "L";
    m_unitAbbreviations["liters"] = "L";
    m_unitAbbreviations["litre"] = "L";
    m_unitAbbreviations["litres"] = "L";
    m_unitAbbreviations["l"] = "L";
    m_unitAbbreviations["L"] = "L";
    m_unitAbbreviations["gallon"] = "gal";
    m_unitAbbreviations["gallons"] = "gal";
    m_unitAbbreviations["gal"] = "gal";
    m_unitAbbreviations["quart"] = "qt";
    m_unitAbbreviations["quarts"] = "qt";
    m_unitAbbreviations["qt"] = "qt";
    m_unitAbbreviations["pint"] = "pt";
    m_unitAbbreviations["pints"] = "pt";
    m_unitAbbreviations["pt"] = "pt";
    m_unitAbbreviations["cup"] = "cup";
    m_unitAbbreviations["cups"] = "cup";
    m_unitAbbreviations["milliliter"] = "mL";
    m_unitAbbreviations["milliliters"] = "mL";
    m_unitAbbreviations["ml"] = "mL";
    m_unitAbbreviations["mL"] = "mL";
    m_unitAbbreviations["fluid ounce"] = "fl oz";
    m_unitAbbreviations["fluid ounces"] = "fl oz";
    m_unitAbbreviations["fl oz"] = "fl oz";
    m_unitAbbreviations["floz"] = "fl oz";

    // Cubic volume units
    m_unitAbbreviations["cubic meter"] = "m³";
    m_unitAbbreviations["cubic meters"] = "m³";
    m_unitAbbreviations["cu m"] = "m³";
    m_unitAbbreviations["m³"] = "m³";
    m_unitAbbreviations["m^3"] = "m³";

    m_unitAbbreviations["cubic foot"] = "ft³";
    m_unitAbbreviations["cubic feet"] = "ft³";
    m_unitAbbreviations["cu ft"] = "ft³";
    m_unitAbbreviations["ft³"] = "ft³";
    m_unitAbbreviations["ft^3"] = "ft³";

    m_unitAbbreviations["cubic inch"] = "in³";
    m_unitAbbreviations["cubic inches"] = "in³";
    m_unitAbbreviations["cu in"] = "in³";
    m_unitAbbreviations["in³"] = "in³";
    m_unitAbbreviations["in^3"] = "in³";

    m_unitAbbreviations["cubic yard"] = "yd³";
    m_unitAbbreviations["cubic yards"] = "yd³";
    m_unitAbbreviations["cu yd"] = "yd³";
    m_unitAbbreviations["yd³"] = "yd³";
    m_unitAbbreviations["yd^3"] = "yd³";

    m_unitAbbreviations["cubic centimeter"] = "cm³";
    m_unitAbbreviations["cubic centimeters"] = "cm³";
    m_unitAbbreviations["cu cm"] = "cm³";
    m_unitAbbreviations["cm³"] = "cm³";
    m_unitAbbreviations["cm^3"] = "cm³";
    m_unitAbbreviations["cc"] = "cm³";

    // Display names
    m_displayNames["L"] = "liters";
    m_displayNames["gal"] = "gallons";
    m_displayNames["qt"] = "quarts";
    m_displayNames["pt"] = "pints";
    m_displayNames["cup"] = "cups";
    m_displayNames["mL"] = "milliliters";
    m_displayNames["fl oz"] = "fluid ounces";
    m_displayNames["m³"] = "cubic meters";
    m_displayNames["ft³"] = "cubic feet";
    m_displayNames["in³"] = "cubic inches";
    m_displayNames["yd³"] = "cubic yards";
    m_displayNames["cm³"] = "cubic centimeters";

    // Categories
    m_unitCategories["L"] = "volume";
    m_unitCategories["gal"] = "volume";
    m_unitCategories["qt"] = "volume";
    m_unitCategories["pt"] = "volume";
    m_unitCategories["cup"] = "volume";
    m_unitCategories["mL"] = "volume";
    m_unitCategories["fl oz"] = "volume";
    m_unitCategories["m³"] = "volume";
    m_unitCategories["ft³"] = "volume";
    m_unitCategories["in³"] = "volume";
    m_unitCategories["yd³"] = "volume";
    m_unitCategories["cm³"] = "volume";

    // Conversion factors to liters
    m_volumeFactors["L"] = 1.0;
    m_volumeFactors["gal"] = 3.78541;  // US gallon
    m_volumeFactors["qt"] = 0.946353;  // US quart
    m_volumeFactors["pt"] = 0.473176;  // US pint
    m_volumeFactors["cup"] = 0.236588; // US cup
    m_volumeFactors["mL"] = 0.001;
    m_volumeFactors["fl oz"] = 0.0295735; // US fluid ounce

    // Cubic volume units (converted to liters)
    m_volumeFactors["m³"] = 1000.0;      // 1 m³ = 1,000 L
    m_volumeFactors["ft³"] = 28.3168;    // 1 ft³ = 28.3168 L
    m_volumeFactors["in³"] = 0.0163871;  // 1 in³ = 0.0163871 L
    m_volumeFactors["yd³"] = 764.555;    // 1 yd³ = 764.555 L
    m_volumeFactors["cm³"] = 0.001;      // 1 cm³ = 0.001 L (same as mL)
}

void UnitConverter::initializeTemperatureUnits()
{
    // Temperature unit abbreviations
    m_unitAbbreviations["celsius"] = "C";
    m_unitAbbreviations["c"] = "C";
    m_unitAbbreviations["C"] = "C";  // Uppercase C
    m_unitAbbreviations["fahrenheit"] = "F";
    m_unitAbbreviations["f"] = "F";
    m_unitAbbreviations["F"] = "F";  // Uppercase F
    m_unitAbbreviations["kelvin"] = "K";
    m_unitAbbreviations["k"] = "K";
    m_unitAbbreviations["K"] = "K";  // Uppercase K

    // Display names
    m_displayNames["C"] = "Celsius";
    m_displayNames["F"] = "Fahrenheit";
    m_displayNames["K"] = "Kelvin";

    // Categories
    m_unitCategories["C"] = "temperature";
    m_unitCategories["F"] = "temperature";
    m_unitCategories["K"] = "temperature";

    // Temperature units for reference
    m_temperatureUnits["C"] = "Celsius";
    m_temperatureUnits["F"] = "Fahrenheit";
    m_temperatureUnits["K"] = "Kelvin";
}

void UnitConverter::initializeTimeUnits()
{
    // Time unit abbreviations
    m_unitAbbreviations["second"] = "s";
    m_unitAbbreviations["seconds"] = "s";
    m_unitAbbreviations["s"] = "s";
    m_unitAbbreviations["minute"] = "min";
    m_unitAbbreviations["minutes"] = "min";
    m_unitAbbreviations["min"] = "min";
    m_unitAbbreviations["hour"] = "h";
    m_unitAbbreviations["hours"] = "h";
    m_unitAbbreviations["h"] = "h";
    m_unitAbbreviations["day"] = "d";
    m_unitAbbreviations["days"] = "d";
    m_unitAbbreviations["d"] = "d";
    m_unitAbbreviations["week"] = "w";
    m_unitAbbreviations["weeks"] = "w";
    m_unitAbbreviations["w"] = "w";
    
    // Display names
    m_displayNames["s"] = "seconds";
    m_displayNames["min"] = "minutes";
    m_displayNames["h"] = "hours";
    m_displayNames["d"] = "days";
    m_displayNames["w"] = "weeks";
    
    // Categories
    m_unitCategories["s"] = "time";
    m_unitCategories["min"] = "time";
    m_unitCategories["h"] = "time";
    m_unitCategories["d"] = "time";
    m_unitCategories["w"] = "time";
    
    // Conversion factors to seconds
    m_timeFactors["s"] = 1.0;
    m_timeFactors["min"] = 60.0;
    m_timeFactors["h"] = 3600.0;
    m_timeFactors["d"] = 86400.0;
    m_timeFactors["w"] = 604800.0;
}

void UnitConverter::initializePowerUnits()
{
    // Power unit abbreviations
    m_unitAbbreviations["watt"] = "W";
    m_unitAbbreviations["watts"] = "W";
    m_unitAbbreviations["w"] = "W";
    m_unitAbbreviations["W"] = "W";

    m_unitAbbreviations["kilowatt"] = "kW";
    m_unitAbbreviations["kilowatts"] = "kW";
    m_unitAbbreviations["kw"] = "kW";
    m_unitAbbreviations["kW"] = "kW";

    m_unitAbbreviations["horsepower"] = "hp";
    m_unitAbbreviations["hp"] = "hp";
    m_unitAbbreviations["HP"] = "hp";

    m_unitAbbreviations["british thermal unit per hour"] = "BTU/h";
    m_unitAbbreviations["btu per hour"] = "BTU/h";
    m_unitAbbreviations["btu/h"] = "BTU/h";
    m_unitAbbreviations["BTU/h"] = "BTU/h";

    // Display names
    m_displayNames["W"] = "watts";
    m_displayNames["kW"] = "kilowatts";
    m_displayNames["hp"] = "horsepower";
    m_displayNames["BTU/h"] = "BTU per hour";

    // Categories
    m_unitCategories["W"] = "power";
    m_unitCategories["kW"] = "power";
    m_unitCategories["hp"] = "power";
    m_unitCategories["BTU/h"] = "power";

    // Conversion factors to watts
    m_powerFactors["W"] = 1.0;
    m_powerFactors["kW"] = 1000.0;
    m_powerFactors["hp"] = 745.7;  // Mechanical horsepower
    m_powerFactors["BTU/h"] = 0.293071;  // BTU per hour to watts
}

void UnitConverter::initializeEnergyUnits()
{
    // Energy unit abbreviations
    m_unitAbbreviations["joule"] = "J";
    m_unitAbbreviations["joules"] = "J";
    m_unitAbbreviations["j"] = "J";
    m_unitAbbreviations["J"] = "J";

    m_unitAbbreviations["kilojoule"] = "kJ";
    m_unitAbbreviations["kilojoules"] = "kJ";
    m_unitAbbreviations["kj"] = "kJ";
    m_unitAbbreviations["kJ"] = "kJ";

    m_unitAbbreviations["calorie"] = "cal";
    m_unitAbbreviations["calories"] = "cal";
    m_unitAbbreviations["cal"] = "cal";

    m_unitAbbreviations["kilocalorie"] = "kcal";
    m_unitAbbreviations["kilocalories"] = "kcal";
    m_unitAbbreviations["kcal"] = "kcal";

    m_unitAbbreviations["british thermal unit"] = "BTU";
    m_unitAbbreviations["btu"] = "BTU";
    m_unitAbbreviations["BTU"] = "BTU";

    m_unitAbbreviations["kilowatt hour"] = "kWh";
    m_unitAbbreviations["kilowatt hours"] = "kWh";
    m_unitAbbreviations["kwh"] = "kWh";
    m_unitAbbreviations["kWh"] = "kWh";

    // Display names
    m_displayNames["J"] = "joules";
    m_displayNames["kJ"] = "kilojoules";
    m_displayNames["cal"] = "calories";
    m_displayNames["kcal"] = "kilocalories";
    m_displayNames["BTU"] = "British thermal units";
    m_displayNames["kWh"] = "kilowatt hours";

    // Categories
    m_unitCategories["J"] = "energy";
    m_unitCategories["kJ"] = "energy";
    m_unitCategories["cal"] = "energy";
    m_unitCategories["kcal"] = "energy";
    m_unitCategories["BTU"] = "energy";
    m_unitCategories["kWh"] = "energy";

    // Conversion factors to joules
    m_energyFactors["J"] = 1.0;
    m_energyFactors["kJ"] = 1000.0;
    m_energyFactors["cal"] = 4.184;  // Thermochemical calorie
    m_energyFactors["kcal"] = 4184.0;  // Kilocalorie
    m_energyFactors["BTU"] = 1055.06;  // International Table BTU
    m_energyFactors["kWh"] = 3600000.0;  // Kilowatt hour to joules
}

void UnitConverter::initializeVelocityUnits()
{
    // Velocity unit abbreviations
    m_unitAbbreviations["meters per second"] = "m/s";
    m_unitAbbreviations["metres per second"] = "m/s";
    m_unitAbbreviations["m/s"] = "m/s";
    m_unitAbbreviations["mps"] = "m/s";

    m_unitAbbreviations["kilometers per hour"] = "km/h";
    m_unitAbbreviations["kilometres per hour"] = "km/h";
    m_unitAbbreviations["km/h"] = "km/h";
    m_unitAbbreviations["kph"] = "km/h";

    m_unitAbbreviations["miles per hour"] = "mph";
    m_unitAbbreviations["mph"] = "mph";
    m_unitAbbreviations["MPH"] = "mph";

    m_unitAbbreviations["feet per second"] = "ft/s";
    m_unitAbbreviations["ft/s"] = "ft/s";
    m_unitAbbreviations["fps"] = "ft/s";

    m_unitAbbreviations["knot"] = "kn";
    m_unitAbbreviations["knots"] = "kn";
    m_unitAbbreviations["kn"] = "kn";
    m_unitAbbreviations["kt"] = "kn";

    m_unitAbbreviations["mach"] = "Ma";
    m_unitAbbreviations["Ma"] = "Ma";

    // Display names
    m_displayNames["m/s"] = "meters per second";
    m_displayNames["km/h"] = "kilometers per hour";
    m_displayNames["mph"] = "miles per hour";
    m_displayNames["ft/s"] = "feet per second";
    m_displayNames["kn"] = "knots";
    m_displayNames["Ma"] = "mach";

    // Categories
    m_unitCategories["m/s"] = "velocity";
    m_unitCategories["km/h"] = "velocity";
    m_unitCategories["mph"] = "velocity";
    m_unitCategories["ft/s"] = "velocity";
    m_unitCategories["kn"] = "velocity";
    m_unitCategories["Ma"] = "velocity";

    // Conversion factors to meters per second
    m_velocityFactors["m/s"] = 1.0;
    m_velocityFactors["km/h"] = 0.277778;  // km/h to m/s
    m_velocityFactors["mph"] = 0.44704;    // mph to m/s
    m_velocityFactors["ft/s"] = 0.3048;    // ft/s to m/s
    m_velocityFactors["kn"] = 0.514444;    // knots to m/s
    m_velocityFactors["Ma"] = 343.0;       // Mach 1 at sea level (approximate)
}

void UnitConverter::initializePressureUnits()
{
    // Pressure unit abbreviations
    m_unitAbbreviations["pascal"] = "Pa";
    m_unitAbbreviations["pascals"] = "Pa";
    m_unitAbbreviations["pa"] = "Pa";
    m_unitAbbreviations["Pa"] = "Pa";

    m_unitAbbreviations["kilopascal"] = "kPa";
    m_unitAbbreviations["kilopascals"] = "kPa";
    m_unitAbbreviations["kpa"] = "kPa";
    m_unitAbbreviations["kPa"] = "kPa";

    m_unitAbbreviations["bar"] = "bar";
    m_unitAbbreviations["bars"] = "bar";

    m_unitAbbreviations["atmosphere"] = "atm";
    m_unitAbbreviations["atmospheres"] = "atm";
    m_unitAbbreviations["atm"] = "atm";

    m_unitAbbreviations["pounds per square inch"] = "psi";
    m_unitAbbreviations["psi"] = "psi";
    m_unitAbbreviations["PSI"] = "psi";

    m_unitAbbreviations["torr"] = "Torr";
    m_unitAbbreviations["Torr"] = "Torr";

    m_unitAbbreviations["millimeter of mercury"] = "mmHg";
    m_unitAbbreviations["millimeters of mercury"] = "mmHg";
    m_unitAbbreviations["mmhg"] = "mmHg";
    m_unitAbbreviations["mmHg"] = "mmHg";

    // Display names
    m_displayNames["Pa"] = "pascals";
    m_displayNames["kPa"] = "kilopascals";
    m_displayNames["bar"] = "bar";
    m_displayNames["atm"] = "atmospheres";
    m_displayNames["psi"] = "pounds per square inch";
    m_displayNames["Torr"] = "torr";
    m_displayNames["mmHg"] = "millimeters of mercury";

    // Categories
    m_unitCategories["Pa"] = "pressure";
    m_unitCategories["kPa"] = "pressure";
    m_unitCategories["bar"] = "pressure";
    m_unitCategories["atm"] = "pressure";
    m_unitCategories["psi"] = "pressure";
    m_unitCategories["Torr"] = "pressure";
    m_unitCategories["mmHg"] = "pressure";

    // Conversion factors to pascals
    m_pressureFactors["Pa"] = 1.0;
    m_pressureFactors["kPa"] = 1000.0;
    m_pressureFactors["bar"] = 100000.0;      // 1 bar = 100,000 Pa
    m_pressureFactors["atm"] = 101325.0;      // 1 atmosphere = 101,325 Pa
    m_pressureFactors["psi"] = 6894.76;       // 1 psi = 6,894.76 Pa
    m_pressureFactors["Torr"] = 133.322;      // 1 Torr = 133.322 Pa
    m_pressureFactors["mmHg"] = 133.322;      // 1 mmHg = 133.322 Pa (same as Torr)
}

double UnitConverter::convertWithinCategory(double value, const QString &fromUnit,
                                           const QString &toUnit, const QString &category)
{
    if (category == "distance") {
        if (!m_distanceFactors.contains(fromUnit) || !m_distanceFactors.contains(toUnit)) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        // Convert to base unit (meters) then to target unit
        double baseValue = value * m_distanceFactors[fromUnit];
        return baseValue / m_distanceFactors[toUnit];
    }
    else if (category == "area") {
        if (!m_areaFactors.contains(fromUnit) || !m_areaFactors.contains(toUnit)) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        // Convert to base unit (square meters) then to target unit
        double baseValue = value * m_areaFactors[fromUnit];
        return baseValue / m_areaFactors[toUnit];
    }
    else if (category == "weight") {
        if (!m_weightFactors.contains(fromUnit) || !m_weightFactors.contains(toUnit)) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        // Convert to base unit (kilograms) then to target unit
        double baseValue = value * m_weightFactors[fromUnit];
        return baseValue / m_weightFactors[toUnit];
    }
    else if (category == "volume") {
        if (!m_volumeFactors.contains(fromUnit) || !m_volumeFactors.contains(toUnit)) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        // Convert to base unit (liters) then to target unit
        double baseValue = value * m_volumeFactors[fromUnit];
        return baseValue / m_volumeFactors[toUnit];
    }
    else if (category == "time") {
        if (!m_timeFactors.contains(fromUnit) || !m_timeFactors.contains(toUnit)) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        // Convert to base unit (seconds) then to target unit
        double baseValue = value * m_timeFactors[fromUnit];
        return baseValue / m_timeFactors[toUnit];
    }
    else if (category == "power") {
        if (!m_powerFactors.contains(fromUnit) || !m_powerFactors.contains(toUnit)) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        // Convert to base unit (watts) then to target unit
        double baseValue = value * m_powerFactors[fromUnit];
        return baseValue / m_powerFactors[toUnit];
    }
    else if (category == "energy") {
        if (!m_energyFactors.contains(fromUnit) || !m_energyFactors.contains(toUnit)) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        // Convert to base unit (joules) then to target unit
        double baseValue = value * m_energyFactors[fromUnit];
        return baseValue / m_energyFactors[toUnit];
    }
    else if (category == "velocity") {
        if (!m_velocityFactors.contains(fromUnit) || !m_velocityFactors.contains(toUnit)) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        // Convert to base unit (meters per second) then to target unit
        double baseValue = value * m_velocityFactors[fromUnit];
        return baseValue / m_velocityFactors[toUnit];
    }
    else if (category == "pressure") {
        if (!m_pressureFactors.contains(fromUnit) || !m_pressureFactors.contains(toUnit)) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        // Convert to base unit (pascals) then to target unit
        double baseValue = value * m_pressureFactors[fromUnit];
        return baseValue / m_pressureFactors[toUnit];
    }

    return std::numeric_limits<double>::quiet_NaN();
}

double UnitConverter::convertTemperature(double value, const QString &fromUnit, const QString &toUnit)
{
    // Handle temperature conversions with offset formulas
    if (fromUnit == toUnit) {
        return value;
    }

    // Convert to Celsius first, then to target unit
    double celsius = value;

    // Convert from source unit to Celsius
    if (fromUnit == "F") {
        celsius = (value - 32.0) * 5.0 / 9.0;
    } else if (fromUnit == "K") {
        celsius = value - 273.15;
    }
    // If fromUnit is "C", celsius is already correct

    // Convert from Celsius to target unit
    if (toUnit == "F") {
        return celsius * 9.0 / 5.0 + 32.0;
    } else if (toUnit == "K") {
        return celsius + 273.15;
    } else if (toUnit == "C") {
        return celsius;
    }

    return std::numeric_limits<double>::quiet_NaN();
}

QString UnitConverter::normalizeUnit(const QString &unit) const
{
    return m_unitAbbreviations.value(unit.toLower(), QString());
}

QString UnitConverter::getUnitCategory(const QString &unit) const
{
    return m_unitCategories.value(unit, QString());
}

// ICalculator interface implementation
CalcForgeResult<QString> UnitConverter::calculate(const QString& expression)
{
    UnitConversionResult result = convertExpression(expression);
    
    if (result.isValid()) {
        auto unitPair = result.value();
        QString formattedResult = QString("%1 %2").arg(unitPair.first).arg(unitPair.second);
        return CalcForgeResult<QString>::success(formattedResult);
    } else {
        return CalcForgeResult<QString>::error(result.errorMessage());
    }
}

QString UnitConverter::getType() const
{
    return "UnitConverter";
}

bool UnitConverter::canHandle(const QString& expression) const
{
    // Check if expression matches unit conversion pattern "X unit to unit"
    QRegularExpression pattern(R"(^[\d.]+\s+.+?\s+to\s+.+?$)", QRegularExpression::CaseInsensitiveOption);
    return pattern.match(expression.trimmed()).hasMatch();
}

QString UnitConverter::getDescription() const
{
    return "Converts between different units of measurement (length, area, weight, volume, temperature, time)";
}

int UnitConverter::getPriority() const
{
    return 10; // High priority - unit conversion should be checked early
}

QStringList UnitConverter::getExamples() const
{
    return {
        "5 feet to meters",
        "100 fahrenheit to celsius", 
        "2.5 miles to kilometers",
        "1000 square feet to square meters",
        "500 grams to ounces",
        "2 hours to minutes"
    };
}
