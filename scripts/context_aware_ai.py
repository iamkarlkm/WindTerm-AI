#!/usr/bin/env python3
import asyncio, websockets, json

class AiServer:
    def __init__(self): self.context = {}
    async def handle(self, ws, path):
        print(f"Client: {ws.remote_address}")
        try:
            async for msg in ws:
                data = json.loads(msg)
                if data["type"] == "context": self.context = data; await ws.send(json.dumps({"status": "ok"}))
                elif data["type"] == "prompt": await ws.send(json.dumps({"response": f"[AI] {data['text']}"}))
        except Exception as e: print(f"Error: {e}")

async def main():
    server = await websockets.serve(AiServer().handle, "127.0.0.1", 8766)
    print("AI WebSocket: ws://127.0.0.1:8766")
    await server.wait_closed()

if __name__ == "__main__": asyncio.run(main())
