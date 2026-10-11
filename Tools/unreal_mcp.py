"""One fixed Unreal project probe over the official MCP stdio protocol."""

import argparse
import asyncio
from datetime import timedelta
import json
from pathlib import Path
import sys

from unreal_mcp_probe import ProbeConfig, ProbeError, failed_probe, run_bounded_probe


CLIENT_TIMEOUT_SECONDS = 55


def create_server(config):
    from mcp.server.fastmcp import FastMCP

    server = FastMCP("Wildlight Project Probe")

    @server.tool()
    async def probe_project() -> dict:
        """Verify the configured open project's identity and engine release."""
        return await asyncio.to_thread(run_bounded_probe, config)

    return server


async def probe_via_mcp(config):
    from mcp import ClientSession, StdioServerParameters
    from mcp.client.stdio import stdio_client

    parameters = StdioServerParameters(command=sys.executable, args=[str(Path(__file__).resolve()),
        "serve", "--engine-root", str(config.engine_root.resolve()),
        "--project-file", str(config.project_file.resolve())])
    async with asyncio.timeout(CLIENT_TIMEOUT_SECONDS):
        async with stdio_client(parameters) as (read, write):
            async with ClientSession(read, write, read_timeout_seconds=timedelta(seconds=CLIENT_TIMEOUT_SECONDS)) as session:
                await session.initialize()
                listed = await session.list_tools()
                if [tool.name for tool in listed.tools] != ["probe_project"]:
                    raise ProbeError("Unexpected MCP tool list")
                schema = listed.tools[0].inputSchema
                if schema.get("properties", {}) or schema.get("required", []):
                    raise ProbeError("Project probe must have no tool arguments")
                response = await session.call_tool("probe_project", arguments={})
                if response.isError:
                    raise ProbeError("MCP probe reported an error")
                payload = response.structuredContent
                if payload is None:
                    texts = [item.text for item in response.content if item.type == "text"]
                    if len(texts) != 1:
                        raise ProbeError("Malformed MCP probe content")
                    payload = json.loads(texts[0])
                if not isinstance(payload, dict) or type(payload.get("connected")) is not bool:
                    raise ProbeError("Malformed MCP probe result")
                return payload


def main(argv=None):
    parser = argparse.ArgumentParser(description="Restricted MCP project identity probe")
    parser.add_argument("command", choices=("serve", "probe"))
    parser.add_argument("--engine-root", required=True, type=Path)
    parser.add_argument("--project-file", required=True, type=Path)
    args = parser.parse_args(argv)
    config = ProbeConfig(args.engine_root.resolve(), args.project_file.resolve())
    try:
        if args.command == "serve":
            create_server(config).run(transport="stdio")
            return 0
        result = asyncio.run(probe_via_mcp(config))
    except Exception as error:
        result = failed_probe(error)
        if args.command == "serve":
            print(json.dumps(result), file=sys.stderr)
            return 1
    print(json.dumps(result))
    return 0 if result.get("connected") is True else 1


if __name__ == "__main__":
    raise SystemExit(main())
