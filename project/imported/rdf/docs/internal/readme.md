# AMD internal: RDF

## Registering a new chunk

RDF files consist of chunks. To ensure that we don't reuse chunk names across tools, we maintain a registry of all chunk names here.

To add a new chunk type, edit the `rdf-registry.json` and add a new entry. You can use the provided schema to validate the JSON (or simply use a schema-enabled editor like Visual Studio Code.)

After editing, re-generate the Markdown registry using `gen_registry.py`.