# VS Code Extension Local Development Guide

This document outlines the successful steps we took to create, test, and install a local VS Code extension without needing to publish it to the marketplace.

## Project Overview
We created a custom VS Code extension called "Augment Right Sidebar" that adds a dedicated panel in the right sidebar for better workspace organization.

## Key Steps That Worked

### 1. Project Setup
```bash
# Create project directory
mkdir augment-right-sidebar
cd augment-right-sidebar

# Initialize npm project
npm init -y

# Install VS Code extension dependencies
npm install --save-dev @types/vscode @types/node eslint typescript @vscode/test-electron
```

### 2. Essential Files Created

#### package.json
- Set `"engines": { "vscode": "^1.74.0" }`
- Added `contributes` section with:
  - `viewsContainers` for activity bar icon
  - `views` for webview panel
  - `commands` for extension commands
- Set `"main": "./extension.js"`

#### extension.js
- Main extension logic with `activate()` and `deactivate()` functions
- Webview provider class for sidebar panel
- Message handlers for webview communication

#### media/right-sidebar-icon.svg
- Custom icon for the activity bar

### 3. Local Installation Process (What Finally Worked)

#### Step 1: Package the Extension
```bash
# Install vsce (VS Code Extension packager) globally
npm install -g vsce

# Package the extension into .vsix file
npx vsce package
```

#### Step 2: Install via VS Code UI
1. Open VS Code
2. Go to Extensions view (Ctrl+Shift+X)
3. Click the "..." menu in the Extensions view
4. Select "Install from VSIX..."
5. Navigate to and select the `.vsix` file
6. Extension installs immediately without requiring marketplace publication

#### Step 3: Verify Installation
- Extension appears in Extensions list
- New icon appears in Activity Bar
- Can be dragged to right sidebar (Secondary Sidebar)

### 4. Key Insights

#### What Didn't Work Initially:
- Trying to use `code --install-extension` command with local files
- Attempting to load extension directly from source folder
- Various command-line installation methods

#### What Finally Worked:
- Using `npx vsce package` to create proper .vsix package
- Installing via VS Code's GUI "Install from VSIX" option
- This method bypasses marketplace entirely

### 5. Extension Features Implemented

#### Core Functionality:
- Custom activity bar icon
- Webview panel in right sidebar
- Iframe integration for external content
- Error handling with fallback options
- Message passing between webview and extension

#### UI Components:
- Loading states
- Error messages with retry functionality
- Responsive design matching VS Code theme
- Proper VS Code styling variables

### 6. Development Workflow

#### For Updates:
1. Modify source files
2. Update version in `package.json`
3. Run `npx vsce package` to create new .vsix
4. Install new version via "Install from VSIX"
5. Reload VS Code window if needed

#### File Structure:
```
augment-right-sidebar/
├── package.json
├── extension.js
├── media/
│   └── right-sidebar-icon.svg
├── node_modules/
└── augment-right-sidebar-x.x.x.vsix
```

### 7. Troubleshooting Tips

#### Common Issues:
- **Extension not appearing**: Check package.json contributes section
- **Icon not showing**: Verify SVG path in viewsContainers
- **Webview not loading**: Check CSP settings and script enablement
- **Updates not taking effect**: Reload VS Code window after installation

#### Debug Methods:
- Use VS Code Developer Tools (Help > Toggle Developer Tools)
- Check extension host logs in Output panel
- Console.log statements in extension.js for debugging

### 8. Alternative Approaches Considered

#### Built-in VS Code Features:
- Moving existing panels to right sidebar (drag & drop)
- Using workspace settings to save layouts
- Leveraging existing extension positioning

#### Custom Extension Benefits:
- Dedicated space for specific tools
- Custom branding and UI
- Integrated functionality
- Persistent positioning

## Conclusion

The key breakthrough was using the VS Code GUI installation method rather than command-line approaches. The `npx vsce package` + "Install from VSIX" workflow provides a clean, reliable way to test local extensions without marketplace publication.

This approach is perfect for:
- Internal company extensions
- Personal productivity tools
- Prototype development
- Custom workspace configurations

## Commands Reference

```bash
# Package extension
npx vsce package

# Install vsce globally (if needed)
npm install -g vsce

# Lint code
npm run lint

# Run tests
npm test
```

## VS Code Installation Steps
1. Extensions view (Ctrl+Shift+X)
2. "..." menu → "Install from VSIX..."
3. Select .vsix file
4. Extension installs immediately
