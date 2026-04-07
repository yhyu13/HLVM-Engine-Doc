# HLVM Engine Console Variable (CVar) System Implementation Plan

## Overview
This document outlines the implementation of a UE5-like console variable (cvar) system with ini-based configuration loading and saving for the HLVM Engine. The system will provide runtime configuration management with persistence to ini files.

## System Architecture

### Core Components

#### 1. Console Variable Types
- **Bool CVar** - Boolean values (true/false, 0/1, on/off)
- **Int CVar** - Integer values (32-bit)
- **Float CVar** - Floating-point values
- **String CVar** - String values

#### 2. Console Variable Manager
- Central registry for all console variables
- Handles registration, lookup, and modification
- Manages loading from and saving to ini files
- Thread-safe operations for runtime modifications

#### 3. Ini Configuration System
- Parser for ini file format (section/key=value)
- Handles multiple ini files with priority hierarchy
- Writes modified values back to appropriate ini files

#### 4. Console Command Interface
- Command-line interface for setting/getting variables
- Integration with engine console system
- Auto-completion and help system

## File Structure

### Directory Layout
```
/Engine/Source/Common/Public/Utility/CVar/
├── CVar.h                     // Main console variable interface and manager
├── CVar.cpp                    // Implementation of console variable system
├── CVarTypes.h                 // Type definitions and templates
├── CVarTypes.cpp               // Type-specific implementations
├── IniParser.h                 // Ini file parsing functionality
├── IniParser.cpp               // Ini parser implementation
├── ConsoleCommand.h             // Console command interface
├── ConsoleCommand.cpp           // Console command implementation
└── CVarMacros.h               // Macros for easy variable registration
```

## Class Design

### 1. ICVar (Interface)
```cpp
class ICVar {
public:
    virtual ~ICVar() = default;
    virtual const FString& GetName() const = 0;
    virtual const FString& GetHelp() const = 0;
    virtual void SetValueFromString(const FString& value) = 0;
    virtual FString GetValueAsString() const = 0;
    virtual void ResetToDefault() = 0;
    virtual bool IsModified() const = 0;
    virtual void ClearModifiedFlag() = 0;
    virtual EConsoleVariableFlags GetFlags() const = 0;
};
```

### 2. CVar Templates
```cpp
template<typename T>
class TTypedCVar : public ICVar {
private:
    FString Name;
    FString Help;
    T DefaultValue;
    T CurrentValue;
    T CachedValue;
    EConsoleVariableFlags Flags;
    bool bModified;
    
public:
    // Constructor with auto-registration
    TTypedCVar(const TCHAR* name, const T& defaultValue, const TCHAR* help, EConsoleVariableFlags flags = EConsoleVariableFlags::None);
    
    // Interface implementation
    const FString& GetName() const override { return Name; }
    const FString& GetHelp() const override { return Help; }
    EConsoleVariableFlags GetFlags() const override { return Flags; }
    
    // Type-specific accessors
    const T& GetValue() const { return CurrentValue; }
    operator T() const { return CurrentValue; }
    void SetValue(const T& newValue);
    
    // String conversion
    void SetValueFromString(const FString& value) override;
    FString GetValueAsString() const override;
    
    // Default/modified handling
    void ResetToDefault() override { CurrentValue = DefaultValue; bModified = false; }
    bool IsModified() const override { return bModified; }
    void ClearModifiedFlag() override { bModified = false; }
};
```

### 3. Console Variable Manager
```cpp
class CVarManager {
private:
    static CVarManager* Singleton;
    std::unordered_map<FString, ICVar*> RegisteredCVars;
    TArray<FString> IniSearchPaths;
    FCriticalSection CVarMutex;
    
    CVarManager();
    
public:
    static CVarManager& Get();
    
    // Registration
    void RegisterCVar(ICVar* cvar);
    ICVar* FindCVar(const FString& name);
    
    // Loading/Saving
    bool LoadFromIni(const FString& iniFile);
    bool SaveToIni(const FString& iniFile);
    void LoadAllFromIni();
    void SaveAllToIni();
    
    // Value operations
    bool SetCVarValue(const FString& name, const FString& value);
    FString GetCVarValue(const FString& name);
    void ResetCVar(const FString& name);
    void ResetAllCVars();
    
    // Console interface
    bool ProcessConsoleCommand(const FString& command);
    void DumpAllCVars();
    void DumpCVarsByCategory(const FString& category);
    
    // Utility
    TArray<ICVar*> GetAllCVars() const;
    TArray<ICVar*> GetCVarsByFlag(EConsoleVariableFlags flag) const;
};
```

### 4. Ini Parser
```cpp
class IniParser {
public:
    struct IniSection {
        FString Name;
        std::unordered_map<FString, FString> KeyValues;
    };
    
private:
    TArray<IniSection> Sections;
    
public:
    bool ParseFile(const FString& filename);
    bool SaveToFile(const FString& filename);
    
    // Accessors
    const TArray<IniSection>& GetSections() const { return Sections; }
    const IniSection* FindSection(const FString& sectionName) const;
    FString GetValue(const FString& section, const FString& key, const FString& defaultValue = TXT("")) const;
    void SetValue(const FString& section, const FString& key, const FString& value);
    void ClearSection(const FString& section);
    void ClearKey(const FString& section, const FString& key);
};
```

## Registration Macros

### Convenience Macros (CVarMacros.h)
```cpp
// Console variable flags
enum class EConsoleVariableFlags : uint32 {
    None = 0,
    Cheat = 1 << 0,           // Marked as cheat (disabled in shipping)
    Saved = 1 << 1,           // Saved to ini file
    RequiresRestart = 1 << 2,   // Requires restart to take effect
    ReadOnly = 1 << 3,          // Read-only after initialization
    Developer = 1 << 4,          // Developer-only variable
    Console = 1 << 5,          // Visible in console
    Archive = 1 << 6            // Archived but not necessarily saved
};

// Auto-registration macros
#define AUTO_CVAR_BOOL(name, defaultValue, help, flags) \
    static TTypedCVar<bool> CVar_##name(TEXT(#name), defaultValue, TEXT(help), flags); \
    static FAutoConsoleVariableRegistrar<bool> Registrar_##name(&CVar_##name);

#define AUTO_CVAR_INT(name, defaultValue, help, flags) \
    static TTypedCVar<int32> CVar_##name(TEXT(#name), defaultValue, TEXT(help), flags); \
    static FAutoConsoleVariableRegistrar<int32> Registrar_##name(&CVar_##name);

#define AUTO_CVAR_FLOAT(name, defaultValue, help, flags) \
    static TTypedCVar<float> CVar_##name(TEXT(#name), defaultValue, TEXT(help), flags); \
    static FAutoConsoleVariableRegistrar<float> Registrar_##name(&CVar_##name);

#define AUTO_CVAR_STRING(name, defaultValue, help, flags) \
    static TTypedCVar<FString> CVar_##name(TEXT(#name), defaultValue, TEXT(help), flags); \
    static FAutoConsoleVariableRegistrar<FString> Registrar_##name(&CVar_##name);

// Reference variable that binds to external variable
#define AUTO_CVAR_REF_BOOL(name, refVar, help, flags) \
    static FAutoConsoleVariableRef<bool> CVarRef_##name(TEXT(#name), refVar, TEXT(help), flags);

#define AUTO_CVAR_REF_INT(name, refVar, help, flags) \
    static FAutoConsoleVariableRef<int32> CVarRef_##name(TEXT(#name), refVar, TEXT(help), flags);
```

## Ini File Structure

### Default Ini Files
- `Engine.ini` - Engine-level settings
- `Game.ini` - Game-specific settings
- `Input.ini` - Input-related settings
- `System.ini` - System configuration

### Ini File Format
```ini
[/Script/Engine.Renderer]
r.VSync=1
r.GBufferFormat=2
r.MaxAnisotropy=8

[/Script/Engine.Engine]
bUseOnScreenDebugMessages=True
FrameRateLimit=60

[/Script/Game.BaseGame]
bShowFPS=False
DifficultyLevel=1
PlayerSpeedMultiplier=1.2
```

### File Hierarchy and Priority
1. Base/DefaultEngine.ini (engine defaults)
2. Engine.ini (user/engine overrides)
3. Base/DefaultGame.ini (game defaults)
4. Game.ini (user/game overrides)

Higher priority files override values from lower priority files.

## Implementation Steps

### Phase 1: Core Infrastructure
1. Create `ICVar` interface and `TTypedCVar` template
2. Implement basic console variable types
3. Create `CVarManager` singleton
4. Implement auto-registration system

### Phase 2: Ini File Support
1. Create `IniParser` class
2. Implement file reading/writing
3. Integrate ini loading/saving with CVarManager
4. Handle file hierarchy and priority

### Phase 3: Console Integration
1. Create `ConsoleCommand` interface
2. Implement console variable commands (set, get, dump)
3. Add help system and auto-completion
4. Integrate with existing engine console

### Phase 4: Advanced Features
1. Add reference variable support
2. Implement change notifications/callbacks
3. Add validation and clamping
4. Create category/group system

## Usage Examples

### Creating Console Variables
```cpp
// In a header file
AUTO_CVAR_BOOL(r_VSync, true, "Enable vertical sync", EConsoleVariableFlags::Saved)
AUTO_CVAR_INT(r_MaxAnisotropy, 8, "Maximum anisotropic filtering", EConsoleVariableFlags::Saved)
AUTO_CVAR_FLOAT(r_ScreenPercentage, 100.0f, "Screen percentage scale", EConsoleVariableFlags::Saved)

// Reference variable
extern bool bUsePostProcessing;
AUTO_CVAR_REF_BOOL(r_PostProcessing, bUsePostProcessing, "Enable post-processing effects", EConsoleVariableFlags::Saved)
```

### Runtime Usage
```cpp
// Get values
if (CVar_r_VSync) {
    // VSync is enabled
}

int32 maxAniso = CVar_r_MaxAnisotropy;

// Set values
CVarManager::Get().SetCVarValue(TEXT("r_VSync"), TEXT("0"));

// Reset to default
CVarManager::Get().ResetCVar(TEXT("r_ScreenPercentage"));
```

### Console Commands
```
# Set value
r.VSync 0

# Get value
r.VSync
# Output: r.VSync = 0 (default: 1)

# Dump all variables
dumpCVars

# Dump by category
dumpCVars renderer

# Reset variable
resetCVar r.VSync

# Reset all
resetAllCVars
```

## Integration Points

### Engine Integration
1. Initialize `CVarManager` early in engine startup
2. Load ini files before subsystem initialization
3. Provide API access for other engine systems
4. Save modified CVars on shutdown

### Editor Integration
1. Add console variable editor UI
2. Provide live modification capabilities
3. Add export/import functionality
4. Integrate with project settings

### Platform Integration
1. Handle platform-specific ini paths
2. Manage read/write permissions
3. Deal with platform-specific constraints

## Performance Considerations

1. **Fast Access**: Console variables should be fast to read (inline getters)
2. **String Conversion**: Minimize string conversions (cache when possible)
3. **Thread Safety**: Use lock-free operations for reads, minimal locking for writes
4. **Memory Usage**: Optimize for large numbers of CVars
5. **File I/O**: Batch ini operations to minimize disk access

## Security Considerations

1. **Cheat Variables**: Mark and disable cheat variables in shipping builds
2. **Validation**: Validate input values and clamp to valid ranges
3. **Sandboxing**: Restrict file I/O to appropriate directories
4. **Access Control**: Implement read-only and developer-only flags

## Testing Strategy

1. **Unit Tests**: Test all CVar types and operations
2. **Integration Tests**: Test ini loading/saving scenarios
3. **Performance Tests**: Measure access performance
4. **Stress Tests**: Test with large numbers of CVars
5. **Platform Tests**: Verify behavior on different platforms

## Future Enhancements

1. **Hot-reloading**: Watch ini files for external changes
2. **Networking**: Sync CVars across network sessions
3. **Profiles**: Support multiple configuration profiles
4. **Metadata**: Add additional metadata (min/max, step, etc.)
5. **Conditional Loading**: Load CVars based on conditions (platform, build config, etc.)
6. **UI Integration**: Native editor interface for managing CVars