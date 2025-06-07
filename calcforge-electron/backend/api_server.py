"""
CalcForge API Server - FastAPI Backend for Electron Frontend
Provides REST API and WebSocket endpoints for real-time calculation.
"""

import asyncio
import json
import re
from typing import Dict, List, Optional, Any
from datetime import datetime

from fastapi import FastAPI, WebSocket, WebSocketDisconnect, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel

from calcforge_engine import CalcForgeEngine


# =============================================================================
# REQUEST/RESPONSE MODELS
# =============================================================================

class CalculationRequest(BaseModel):
    expression: str
    sheet_id: int = 0
    line_num: int = 1


class CalculationResponse(BaseModel):
    value: Any
    unit: str = ""
    error: Optional[str] = None
    highlights: List[Dict] = []


class BatchCalculationRequest(BaseModel):
    expressions: List[str]
    sheet_id: int = 0


class BatchCalculationResponse(BaseModel):
    results: List[CalculationResponse]


class SyntaxHighlightRequest(BaseModel):
    text: str


class SyntaxHighlightResponse(BaseModel):
    highlights: List[Dict]


class WorksheetData(BaseModel):
    sheet_id: int
    name: str
    content: str


class WorksheetsUpdateRequest(BaseModel):
    worksheets: Dict[int, WorksheetData]


# =============================================================================
# FASTAPI APPLICATION SETUP
# =============================================================================

app = FastAPI(
    title="CalcForge API",
    description="Backend API for CalcForge Electron Application",
    version="4.0.0"
)

# Enable CORS for Electron frontend
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],  # In production, specify exact origins
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Initialize the calculation engine
engine = CalcForgeEngine()

# WebSocket connection manager
class ConnectionManager:
    def __init__(self):
        self.active_connections: List[WebSocket] = []

    async def connect(self, websocket: WebSocket):
        await websocket.accept()
        self.active_connections.append(websocket)

    def disconnect(self, websocket: WebSocket):
        self.active_connections.remove(websocket)

    async def send_personal_message(self, message: str, websocket: WebSocket):
        await websocket.send_text(message)

    async def broadcast(self, message: str):
        for connection in self.active_connections:
            try:
                await connection.send_text(message)
            except:
                # Remove dead connections
                self.active_connections.remove(connection)

manager = ConnectionManager()


# =============================================================================
# API ENDPOINTS
# =============================================================================

@app.get("/")
@app.head("/")
async def root():
    """Health check endpoint"""
    return {
        "message": "CalcForge API Server",
        "version": "4.0.0",
        "status": "running",
        "timestamp": datetime.now().isoformat()
    }


@app.post("/api/calculate", response_model=CalculationResponse)
async def calculate_expression(request: CalculationRequest):
    """
    Calculate a single expression and return the result with syntax highlighting.
    """
    try:
        # Evaluate the expression
        result = engine.evaluate_expression(
            request.expression, 
            request.sheet_id, 
            request.line_num
        )
        
        # Get syntax highlighting
        highlights = engine.get_syntax_highlights(request.expression)
        
        return CalculationResponse(
            value=result["value"],
            unit=result["unit"],
            error=result["error"],
            highlights=highlights
        )
        
    except Exception as e:
        return CalculationResponse(
            value="",
            unit="",
            error=f"Server Error: {str(e)}",
            highlights=[]
        )


@app.post("/api/calculate-batch", response_model=BatchCalculationResponse)
async def calculate_batch(request: BatchCalculationRequest):
    """
    Calculate multiple expressions in batch for efficiency.
    """
    results = []
    
    for i, expression in enumerate(request.expressions):
        try:
            # Evaluate each expression
            result = engine.evaluate_expression(
                expression, 
                request.sheet_id, 
                i + 1  # Line number
            )
            
            # Get syntax highlighting
            highlights = engine.get_syntax_highlights(expression)
            
            results.append(CalculationResponse(
                value=result["value"],
                unit=result["unit"],
                error=result["error"],
                highlights=highlights
            ))
            
        except Exception as e:
            results.append(CalculationResponse(
                value="",
                unit="",
                error=f"Server Error: {str(e)}",
                highlights=[]
            ))
    
    return BatchCalculationResponse(results=results)


@app.post("/api/syntax-highlight", response_model=SyntaxHighlightResponse)
async def get_syntax_highlighting(request: SyntaxHighlightRequest):
    """
    Get syntax highlighting data for text without evaluation.
    REPLACED: Now uses the same working logic as the replacement server.
    """
    try:
        print(f"MAIN API received text: '{request.text}'")

        # Use the WORKING syntax highlighting logic from replacement server
        highlights = get_full_syntax_highlights(request.text)

        print(f"MAIN API returning {len(highlights)} highlights")
        return SyntaxHighlightResponse(highlights=highlights)
    except Exception as e:
        print(f"MAIN API error: {e}")
        return SyntaxHighlightResponse(highlights=[])


@app.post("/api/worksheets/update")
async def update_worksheets(request: WorksheetsUpdateRequest):
    """
    Update worksheet data for cross-sheet reference calculations.
    """
    try:
        # Convert worksheet data to the format expected by the engine
        sheets_data = {}
        for sheet_id, worksheet in request.worksheets.items():
            sheets_data[worksheet.name.lower()] = worksheet.content
        
        # Update the engine's worksheet data
        engine.manage_cross_sheet_refs(sheets_data)
        
        return {"status": "success", "message": "Worksheets updated"}
        
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Error updating worksheets: {str(e)}")


@app.get("/api/functions")
async def get_available_functions():
    """
    Get list of available functions for autocompletion.
    """
    from constants import FUNCTION_NAMES

    functions = []
    for func_name in FUNCTION_NAMES:
        functions.append({
            "name": func_name,
            "description": f"{func_name.upper()} function"
        })

    return {"functions": functions}


@app.post("/api/test-syntax")
async def test_syntax_direct():
    """
    Direct test endpoint that bypasses all existing code.
    """
    print("DEBUG: Direct test endpoint called!")
    return {
        "highlights": [
            {
                "start": 0,
                "length": 20,
                "class": "syntax-comment",
                "color": "#7ED321"  # Force green color
            }
        ]
    }


# =============================================================================
# WEBSOCKET ENDPOINTS
# =============================================================================

@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    """
    WebSocket endpoint for real-time calculation updates.
    """
    await manager.connect(websocket)
    try:
        while True:
            # Receive data from client
            data = await websocket.receive_text()
            message = json.loads(data)
            
            if message["type"] == "calculate":
                # Handle real-time calculation
                result = engine.evaluate_expression(
                    message["expression"],
                    message.get("sheet_id", 0),
                    message.get("line_num", 1)
                )
                
                highlights = engine.get_syntax_highlights(message["expression"])
                
                response = {
                    "type": "calculation_result",
                    "line_num": message.get("line_num", 1),
                    "result": {
                        "value": result["value"],
                        "unit": result["unit"],
                        "error": result["error"],
                        "highlights": highlights
                    }
                }
                
                await manager.send_personal_message(json.dumps(response), websocket)
                
            elif message["type"] == "batch_calculate":
                # Handle batch calculation
                results = []
                for i, expr in enumerate(message["expressions"]):
                    result = engine.evaluate_expression(
                        expr,
                        message.get("sheet_id", 0),
                        i + 1
                    )
                    highlights = engine.get_syntax_highlights(expr)
                    results.append({
                        "value": result["value"],
                        "unit": result["unit"],
                        "error": result["error"],
                        "highlights": highlights
                    })
                
                response = {
                    "type": "batch_calculation_result",
                    "results": results
                }
                
                await manager.send_personal_message(json.dumps(response), websocket)
                
    except WebSocketDisconnect:
        manager.disconnect(websocket)


# =============================================================================
# WORKING SYNTAX HIGHLIGHTING LOGIC (copied from replacement server)
# =============================================================================

# Global LN color mapping (persistent across requests)
persistent_ln_colors = {}

def get_full_syntax_highlights(text):
    """Full syntax highlighting logic that actually works"""

    # Define colors (matching the working replacement server)
    COLORS = {
        'comment': '#7ED321',    # Green comments
        'number': '#FFFFFF',     # White numbers
        'operator': '#BB8FCE',   # Light purple operators
        'function': '#4A90E2',   # Blue functions
        'paren': '#7ED321',      # Green parentheses
        'unmatched': '#F85149',  # Red unmatched parentheses
        'sheet_ref': '#4DA6FF',  # Blue for sheet references
    }

    # LN variable color palette (rotating colors)
    LN_COLORS = [
        '#FF6B6B',  # Red
        '#4ECDC4',  # Teal
        '#45B7D1',  # Blue
        '#96CEB4',  # Green
        '#FFEAA7',  # Yellow
        '#DDA0DD',  # Plum
        '#98D8C8',  # Mint
        '#F7DC6F',  # Light Yellow
        '#BB8FCE',  # Light Purple
        '#85C1E9'   # Light Blue
    ]

    # Function names for highlighting
    FUNCTION_NAMES = [
        'sin', 'cos', 'tan', 'asin', 'acos', 'atan', 'sinh', 'cosh', 'tanh',
        'asinh', 'acosh', 'atanh', 'sqrt', 'log', 'log10', 'log2', 'exp', 'pow',
        'ceil', 'floor', 'abs', 'factorial', 'gcd', 'lcm', 'sum', 'mean', 'median',
        'mode', 'min', 'max', 'count', 'product', 'variance', 'stdev', 'std',
        'range', 'geomean', 'harmmean', 'sumsq', 'perc5', 'perc95', 'meanfps',
        'TC', 'AR', 'D', 'TR', 'truncate', 'pi', 'e'
    ]

    highlights = []

    # Process line by line for proper highlighting
    lines = text.split('\n')
    current_pos = 0

    for line in lines:
        line_start = current_pos
        line_length = len(line)

        # Handle comment lines (full line highlighting)
        if line.strip().startswith(":::"):
            highlights.append({
                "start": line_start,
                "length": line_length,
                "class": "syntax-comment",
                "color": COLORS['comment'],
                "bold": True
            })
            print(f"Found comment line at position {line_start}: '{line}'")
        else:
            # Process non-comment lines for detailed syntax highlighting
            line_highlights = highlight_line_main(line, line_start, COLORS, FUNCTION_NAMES, LN_COLORS)
            highlights.extend(line_highlights)

        # Move to next line (including newline character)
        current_pos += line_length + 1

    return highlights


def highlight_line_main(line, line_start, COLORS, FUNCTION_NAMES, LN_COLORS):
    """Highlight a single line of text with full syntax highlighting"""
    highlights = []

    if not line.strip():
        return highlights

    # Highlight numbers
    for match in re.finditer(r"\b\d+(?:\.\d+)?\b", line):
        highlights.append({
            "start": line_start + match.start(),
            "length": match.end() - match.start(),
            "class": "syntax-number",
            "color": COLORS['number']
        })

    # Highlight operators (including 'to' keyword)
    for match in re.finditer(r"\bto\b|[+\-*/%^=]", line):
        highlights.append({
            "start": line_start + match.start(),
            "length": match.end() - match.start(),
            "class": "syntax-operator",
            "color": COLORS['operator']
        })

    # Highlight function names
    for func_name in FUNCTION_NAMES:
        # Create regex pattern for function name followed by opening parenthesis
        pattern = r"\b" + re.escape(func_name) + r"\b(?=\s*\()"
        for match in re.finditer(pattern, line, re.IGNORECASE):
            highlights.append({
                "start": line_start + match.start(),
                "length": match.end() - match.start(),
                "class": "syntax-function",
                "color": COLORS['function']
            })

    # Highlight parentheses
    stack = []
    pairs = []
    for i, ch in enumerate(line):
        if ch == '(':
            stack.append(i)
        elif ch == ')' and stack:
            start = stack.pop()
            pairs.append((start, i))

    # Highlight matched pairs
    for start, end in pairs:
        highlights.append({
            "start": line_start + start,
            "length": 1,
            "class": "syntax-paren",
            "color": COLORS['paren']
        })
        highlights.append({
            "start": line_start + end,
            "length": 1,
            "class": "syntax-paren",
            "color": COLORS['paren']
        })

    # Highlight unmatched opening parentheses
    for pos in stack:
        highlights.append({
            "start": line_start + pos,
            "length": 1,
            "class": "syntax-unmatched",
            "color": COLORS['unmatched']
        })

    # Highlight LN references (case insensitive)
    ln_pattern = r"\bLN(\d+)\b"
    for match in re.finditer(ln_pattern, line, re.IGNORECASE):
        ln_num = int(match.group(1))
        ln_color = get_ln_color_main(ln_num, LN_COLORS)
        highlights.append({
            "start": line_start + match.start(),
            "length": match.end() - match.start(),
            "class": "syntax-ln-ref",
            "color": ln_color,
            "bold": True,  # LN references are bold
            "ln_number": ln_num
        })

    # Highlight cross-sheet references (case insensitive)
    sheet_pattern = r"\bS\.(.*?)\.LN(\d+)\b"
    for match in re.finditer(sheet_pattern, line, re.IGNORECASE):
        sheet_name = match.group(1)
        ln_num = int(match.group(2))
        ln_color = get_ln_color_main(ln_num, LN_COLORS)

        # Highlight the entire reference
        highlights.append({
            "start": line_start + match.start(),
            "length": match.end() - match.start(),
            "class": "syntax-sheet-ref",
            "color": ln_color,
            "bold": True,
            "ln_number": ln_num,
            "sheet_name": sheet_name
        })

        # Add special highlighting for the sheet name part
        sheet_start = line_start + match.start() + 2  # Skip "S."
        sheet_length = len(sheet_name)
        highlights.append({
            "start": sheet_start,
            "length": sheet_length,
            "class": "syntax-sheet-name",
            "color": COLORS['sheet_ref']
        })

    return highlights


def get_ln_color_main(ln_number, LN_COLORS):
    """Get or assign a color for an LN variable"""
    global persistent_ln_colors

    if ln_number not in persistent_ln_colors:
        # Assign a new color from the palette
        color_idx = len(persistent_ln_colors) % len(LN_COLORS)
        persistent_ln_colors[ln_number] = LN_COLORS[color_idx]
        print(f"Assigned color {LN_COLORS[color_idx]} to LN{ln_number}")

    return persistent_ln_colors[ln_number]


# =============================================================================
# SERVER STARTUP
# =============================================================================

if __name__ == "__main__":
    import uvicorn
    
    print("Starting CalcForge API Server...")
    print("Server will be available at: http://localhost:8000")
    print("API documentation at: http://localhost:8000/docs")
    
    uvicorn.run(
        "api_server:app",
        host="127.0.0.1",
        port=8000,
        reload=True,
        log_level="info"
    )
