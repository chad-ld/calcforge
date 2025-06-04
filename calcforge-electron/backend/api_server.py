"""
CalcForge API Server - FastAPI Backend for Electron Frontend
Provides REST API and WebSocket endpoints for real-time calculation.
"""

import asyncio
import json
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
    """
    try:
        highlights = engine.get_syntax_highlights(request.text)
        return SyntaxHighlightResponse(highlights=highlights)
    except Exception as e:
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
