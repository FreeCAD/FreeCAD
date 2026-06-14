from bridge import mcp, asset_creation_strategy as _asset_creation_strategy


def register() -> None:
    @mcp.prompt()
    def asset_creation_strategy() -> str:
        return _asset_creation_strategy()
