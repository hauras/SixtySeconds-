import sys, time, json
sys.path.insert(0, 'D:/Program Files/Epic Games/UE_5.7/Engine/Plugins/Experimental/PythonScriptPlugin/Content/Python')
import remote_execution as remote
r = remote.RemoteExecution()
try:
    r.start()
    time.sleep(3)
    nodes = [n for n in r.remote_nodes if n.get('project_name') == 'SixtySeconds']
    if len(nodes) != 1:
        raise RuntimeError('Expected one project editor: ' + repr(r.remote_nodes))
    r.open_command_connection(nodes[0]['node_id'])
    result = r.run_command(open(sys.argv[1], encoding='utf-8').read())
    result.pop('command', None)
    print(json.dumps(result, ensure_ascii=False))
    if not result['success']: sys.exit(1)
finally:
    r.stop()
