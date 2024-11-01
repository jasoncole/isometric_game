
// TODO: meta-programming?

internal ll_modifier_event_listener*
Registermodifier_event_listener(arena* ListenerArena, ll_modifier_event_listener* Start, modifier_event_listener* ListenerFunc)
{
    // TODO: circular buffer
    ll_modifier_event_listener* ListenerNode = PushStruct(ListenerArena, ll_modifier_event_listener);
    Start->Prev = ListenerNode;
    ListenerNode->Next = Start;
    ListenerNode->Listener = ListenerFunc;
    return ListenerNode;
}

// TODO: how?
internal void
Removemodifier_event_listener(ll_modifier_event_listener* Node)
{
    if (Node->Prev)
    {
        Node->Prev->Next = Node->Next;
    }
    if (Node->Next)
    {
        Node->Next->Prev = Node->Prev;
    }
}
