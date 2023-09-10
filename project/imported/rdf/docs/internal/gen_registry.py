import json

if __name__=='__main__':
    registry = json.load(open('rdf-registry.json'))
    
    with open('registry.md', 'w') as output:
        output.write('# Chunk type registry\n')
        output.write('Chunk name | Produced by ... | Consumed by ... | Description | Reference\n')
        output.write('---|---|---|---|---\n')
        for entry in sorted(registry['chunkTypes'], key=lambda x: x['name']):
            row = [
                f'`{entry["name"]}`',
                ', '.join(entry['producer']),
                ', '.join(entry['consumer']),
                entry['description'],
                ', '.join(f'[{ref["name"]}]({ref["url"]})' for ref in entry['references'])
            ]
            output.write(' | '.join(row) + '\n')