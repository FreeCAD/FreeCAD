# Repository instructions

- Use the most natural approach to fit new features into existing FreeCAD architecture unless obvious gains are available from other implementation decisions.
- Keep the CadX chat model provider-agnostic: use any locally served Ollama model and do not hard-code a particular model family.
- Present the application to users as `CADX`, while preserving internal FreeCAD executable, library, Python-module, bundle-identifier, and document-format names required for compatibility.
- For CadX builds and UI validation, run `tools/cadx-build-validate.sh` and follow `docs/cadx-build-and-ui-validation.md`. Never test a stale or runtime-incomplete FreeCAD artifact or accept OpenAI/API-key controls in the CadX panel; the supported panel is local Ollama only.
