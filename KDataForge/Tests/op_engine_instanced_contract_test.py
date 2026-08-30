"""Static contract guard for instanced-object array ops.

`mUnlocks`-style arrays hold subobjects that have to be constructed, not object references to
resolve by path. A map node has no path, so before this contract existed the element ops read
`{class: ..., properties: [...]}` as an empty path, wrote null, and reported success — appends
landed as empty entries with a clean `/kdf report`.

No Unreal runtime dependency: this guards the source-level contract until the plugin's automation
suite covers the flow directly. Run with the repository Python interpreter.
"""

from pathlib import Path

ROOT = Path(__file__).parents[1]
OP_ENGINE = ROOT / 'Source/KDataForge/Private/Reflection/KDFOpEngine.cpp'
VALUE_CODEC = ROOT / 'Source/KDataForge/Private/Reflection/KDFValueCodec.cpp'


def require(text: str, expected: str, where: str) -> None:
    assert expected in text, f'Missing required contract in {where}: {expected}'


def test_array_element_ops_construct_instanced_subobjects() -> None:
    source = OP_ENGINE.read_text(encoding='utf-8')

    # append/prepend/insert must have an outer to own the constructed subobject.
    require(source, 'bool ApplyArrayOp(EKDFOp Op, UObject* Outer,', 'KDFOpEngine.cpp')
    require(source, 'ApplyArrayOp(Op, RootObject,', 'KDFOpEngine.cpp')

    # ... and must build the element instead of resolving it as a path.
    require(source, 'MakeInstancedObjectFromNode(Outer, InstancedInner, *Elements[Offset], OutError)',
            'KDFOpEngine.cpp')

    # A TSubclassOf element is a class reference, never a subobject: FClassProperty must be excluded.
    require(source, 'CastField<FClassProperty>(Inner) != nullptr ? nullptr : CastField<FObjectProperty>(Inner)',
            'KDFOpEngine.cpp')

    # `set` and the element ops go through the same constructor.
    assert source.count('MakeInstancedObjectFromNode(') >= 3, 'set and element ops must share one constructor'


def test_object_references_reject_non_scalar_nodes() -> None:
    source = VALUE_CODEC.read_text(encoding='utf-8')

    require(source, 'bool RequireObjectReferenceNode(', 'KDFValueCodec.cpp')
    # Null stays legal (`value: ~` clears a reference); maps and sequences must error.
    require(source, 'if (Node.IsScalar() || Node.IsNull())', 'KDFValueCodec.cpp')
    # Every object-ish branch is guarded, or a map silently nulls the slot again.
    assert source.count('if (!RequireObjectReferenceNode(Node, Property, OutError))') == 4, (
        'object, class, soft object and soft class branches must all be guarded')


if __name__ == '__main__':
    test_array_element_ops_construct_instanced_subobjects()
    test_object_references_reject_non_scalar_nodes()
    print('ok')
