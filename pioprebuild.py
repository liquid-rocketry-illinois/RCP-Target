Import('env')
env.AddCustomTarget(
    name="gen_version",
    dependencies=["VERSION"],
    actions=["python piogenversion.py"],
    title="Generate RCPTVersion"
)