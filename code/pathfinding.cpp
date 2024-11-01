
// TODO:
// how often should the actor recalculate its path if it can't reach its destination?
// use chunks to pathfind over large distances?
// consider Z

// BUG : move to the left, hold move, then go up - player will keep moving left for some reason
// the move target moves to the correct location so seems to be a problem with pathfinding
// i think its a problem with moveentity. when there was an old path in the patharena, it was finishing that path before making a new one
// should i just recalculate the path every frame?
// whatever lets just switch to wave function pathfinding zzz

struct path_node
{
    sv2 position;
    f32 cost; // cost to get to this node
    path_node* Origin; // node used to get to this node in shortest known path
};

#define SQRT_2 1.41421356237f
#define MAX_NODES 100
#define PATH_ARENA_SIZE (sizeof(sv2)*MAX_NODES)


internal f32
CalculateGridDistance(sv2 Start, sv2 End)
{
    sv2 Delta = End - Start;
    Delta.x = ABS(Delta.x);
    Delta.y = ABS(Delta.y);
    
    s32 AdjacentCount = ABS(Delta.x - Delta.y);
    s32 DiagonalCount = MAX(Delta.x, Delta.y) - AdjacentCount;
    
    return (f32)AdjacentCount + (SQRT_2 * (f32)DiagonalCount);
}

// NOTE: path is written in reverse order
internal void
WriteFullPath(arena* WriteArena, path_node* Goal)
{
    if (Goal == 0)
    {
        return;
    }
    
    // NOTE: start node is intentionally excluded
    path_node* CurrentNode = Goal;
    while (CurrentNode->Origin)
    {
        sv2* Record = PushStruct(WriteArena, sv2);
        Record->x = CurrentNode->position.x;
        Record->y = CurrentNode->position.y;
        
        CurrentNode = CurrentNode->Origin;
    }
}

// NOTE: sorted worst to best
internal void
SortedInsert(path_node* Node, 
             path_node** Array, u32* Count)
{
    for (u32 FindInsert = 0;
         FindInsert < *Count;
         ++FindInsert)
    {
        if (Array[FindInsert]->cost <= Node->cost)
        {
            for (u32 PushBack = *Count;
                 PushBack > FindInsert;
                 --PushBack)
            {
                Array[PushBack] = Array[PushBack - 1];
            }
            Array[FindInsert] = Node;
            (*Count)++;
            return;
        }
    }
    Array[*Count] = Node;
    (*Count)++;
}

internal void
RemoveNode(path_node* Node, path_node** Array, u32* Count)
{
    for (u32 Find = 0;
         Find < *Count;
         ++Find)
    {
        if (Array[Find] == Node)
        {
            for (u32 Replace = Find;
                 Replace < *Count - 1;
                 ++Replace)
            {
                Array[Replace] = Array[Replace + 1];
            }
            (*Count)--;
        }
    }
}

s32
FindObstacle(sv2 Position, sv2* Obstacles, s32 ObstacleCount)
{
    for (s32 ObstacleIter = 0;
         ObstacleIter < ObstacleCount;
         ++ObstacleIter)
    {
        if (Obstacles[ObstacleIter] == Position) return ObstacleIter;
    }
    return -1;
}

void
GeneratePath(arena* ScratchArena, arena* WriteArena,
             sv2 Start, sv2 End,
             sv2* Obstacles, u32 ObstacleCount)
{
    if (ScratchArena == 0 || WriteArena == 0)
    {
        InvalidCodePath;
        return;
    }
    
    u32 NodeCount = 0;
    path_node* PathNodes = PushArray(ScratchArena, path_node, MAX_NODES);
    
    // NOTE: sorted in reverse order
    u32 OpenCount = 0;
    path_node** OpenNodes = PushArray(ScratchArena, path_node*, MAX_NODES);
    
    u32 ClosedCount = 0;
    path_node** ClosedNodes = PushArray(ScratchArena, path_node*, MAX_NODES);
    
    PathNodes->position.x = Start.x;
    PathNodes->position.y = Start.y;
    PathNodes->cost = 0;
    PathNodes->Origin = 0;
    NodeCount++;
    
    OpenNodes[OpenCount++] = PathNodes;
    
    f32 BestEstimatedDistance = CalculateGridDistance(PathNodes->position, End);
    path_node* BestNode = PathNodes;
    
    // TODO: less hacky
    while (OpenCount > 0 && NodeCount < MAX_NODES - 8)
    {
        path_node* Current = OpenNodes[OpenCount - 1];
        OpenCount--;
        
        if (Current->position == End)
        {
            WriteFullPath(WriteArena, Current);
            return;
        }
        
        // test each neighbor
        for (s32 DeltaX = -1;
             DeltaX < 2;
             ++DeltaX)
        {
            for (s32 DeltaY = -1;
                 DeltaY < 2;
                 ++DeltaY)
            {
                if (DeltaX == 0 && DeltaY == 0) continue;
                
                path_node* Neighbor = PathNodes + NodeCount;
                Neighbor->position.x = Current->position.x + DeltaX;
                Neighbor->position.y = Current->position.y + DeltaY;
                
                if (FindObstacle(Neighbor->position, Obstacles, ObstacleCount) >= 0)
                {
                    continue;
                }
                
                f32 EstimatedDistance = CalculateGridDistance(Neighbor->position, End);
                
                Neighbor->cost = Current->cost + 
                    CalculateGridDistance(Current->position, Neighbor->position) + 
                    EstimatedDistance;
                Neighbor->Origin = Current;
                
                for (u32 OpenIter = 0;
                     OpenIter < OpenCount;
                     ++OpenIter)
                {
                    path_node* TestNode = OpenNodes[OpenIter];
                    if (TestNode->position == Neighbor->position)
                    {
                        if (Neighbor->cost < TestNode->cost) 
                        {
                            TestNode->cost = Neighbor->cost;
                            Neighbor = TestNode;
                        }
                        goto Exit;
                    }
                }
                
                for (u32 ClosedIter = 0;
                     ClosedIter < ClosedCount;
                     ++ClosedIter)
                {
                    path_node* TestNode = ClosedNodes[ClosedIter];
                    if (TestNode->position == Neighbor->position)
                    {
                        if (Neighbor->cost < TestNode->cost) 
                        {
                            TestNode->cost = Neighbor->cost;
                            SortedInsert(TestNode, OpenNodes, &OpenCount);
                            RemoveNode(TestNode, ClosedNodes, &ClosedCount);
                            Neighbor = TestNode;
                        }
                        goto Exit;
                    }
                }
                // Add Neighbor
                SortedInsert(Neighbor, OpenNodes, &OpenCount);
                NodeCount++;
                
                Exit: 
                // track best node so far
                if (EstimatedDistance < BestEstimatedDistance)
                {
                    BestNode = Neighbor;
                    BestEstimatedDistance = EstimatedDistance;
                }
            }
        }
        
        // add current node to closed
        ClosedNodes[ClosedCount++] = Current;
        
        
        
    }
    
    // if no path found, return best path
    WriteFullPath(WriteArena, BestNode);
}
