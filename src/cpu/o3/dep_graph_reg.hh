// Arya's stuff

#ifndef __CPU_O3_DEP_GRAPH_REG_HH__
#define __CPU_O3_DEP_GRAPH_REG_HH__

#include "cpu/o3/comm.hh"

namespace gem5
{

namespace o3
{

template <class DynInstPtr>
class DependencyGraphReg
{
  public:
    // typedef DependencyEntry<DynInstPtr> DepEntry;

    /** Default construction.  Must call resize() prior to use. */
    DependencyGraphReg():
        nodesTraversed(0),
        nodesRemoved(0)
    {
    }

    ~DependencyGraphReg()
    {}

    /** Resize the dependency graph to have num_entries registers. */
    void resize(int num_entries, int num_reservation_stations);

    /** Clears all of the linked lists. */
    void reset();

    /** Inserts an instruction to be dependent on the given index. */
    void insert(RegIndex idx, const DynInstPtr &new_inst);

    /** Sets the producing instruction of a given register. */
    void setInst(RegIndex idx, const DynInstPtr &new_inst);

    /** Clears the producing instruction. */
    void clearInst(RegIndex idx);

    /** Removes an instruction from a single linked list. */
    void remove(RegIndex idx, const DynInstPtr &inst_to_remove);

    void retract(const DynInstPtr &inst);

    /** Removes and returns the newest dependent of a specific register. */
    DynInstPtr pop(RegIndex idx);

    /** Checks if the entire dependency graph is empty. */
    bool empty() const;

    /** Checks if there are any dependents on a specific register. */
    bool empty(RegIndex idx) const;

    /** Debugging function to dump out the dependency graph.
     */
    void dump();

  private:
    // for mapping the sources to reg index
    // for maintaining gem5 compatibility only
    // not needed in actual hardware
    // std::vector<DynInstPtr> sources;

    // actual instruction queue
    std::vector<DynInstPtr> instructions;

    // mapping between registers and the instructions which depend on them
    // A register can be source to one or more instructions in the reservation stations
    // If it's a source, then the flag will be set to true
    // numPhyRegs <-> numReservationStations
    // std::vector<std::vector<bool>> reservationStationMap;

    // A bool map would've been sufficient if not for the gem5 intricacies
    // If an instruction is dependent on the same register for both of it's sources
    // gem5 would require the entry to be present twice at that index
    std::vector<std::vector<int>> reservationStationMap;

    /** Number of linked lists; identical to the number of registers. */
    int numPhyRegs;
    int numReservationStations;

    // Debug variable, remove when done testing.
    // unsigned memAllocCounter;

  public:
    // Debug variable, remove when done testing.
    uint64_t nodesTraversed;
    // Debug variable, remove when done testing.
    uint64_t nodesRemoved;
};

// functions begin
template <class DynInstPtr>
void
DependencyGraphReg<DynInstPtr>::resize(int numPhyRegs, int numReservationStations)
{
    this->numPhyRegs = numPhyRegs;
    this->numReservationStations = numReservationStations;

    instructions.clear();
    reservationStationMap.clear();

    for (int i = 0; i < numPhyRegs; i++) {
        std::vector<int> reservationStationEntry;

        for (int j = 0; j < numReservationStations; j++) {
            reservationStationEntry.push_back(0);
        }

        reservationStationMap.push_back(reservationStationEntry);
    }

    // for (int i = 0; i < numPhyRegs; i++) {
    //     sources.push_back(NULL);
    // }

    for (int i = 0; i < numReservationStations; i++) {
        instructions.push_back(NULL);
    }
}

template <class DynInstPtr>
void
DependencyGraphReg<DynInstPtr>::reset()
{
    // no need to clean anything up
    // everything auto deallocated
}

template <class DynInstPtr>
void
DependencyGraphReg<DynInstPtr>::insert(RegIndex reg_idx, const DynInstPtr &new_inst)
{
    //Add this new, dependent instruction at the head of the dependency
    //chain.

    // First create the entry that will be added to the head of the
    // dependency chain.
    // DepEntry *new_entry = new DepEntry;
    // new_entry->next = dependGraph[idx].next;
    // new_entry->inst = new_inst;

    // Then actually add it to the chain.
    // dependGraph[idx].next = new_entry;

    int new_inst_idx = -1;

    // search for instruction in the existing entries
    for (int idx = 0; idx < numReservationStations; idx++) {
        if (new_inst == instructions[idx]) {
            new_inst_idx = idx;
            break;
        }
    }

    // find an empty spot to insert
    if (-1 == new_inst_idx) {
        for (int idx = 0; idx < numReservationStations; idx++) {
            if (NULL == instructions[idx]) {
                new_inst_idx = idx;
                break;
            }
        }
    }

    assert(new_inst_idx != -1);

    instructions[new_inst_idx] = new_inst;
    reservationStationMap[reg_idx][new_inst_idx]++;

    // ++memAllocCounter;
}

template <class DynInstPtr>
void
DependencyGraphReg<DynInstPtr>::setInst(RegIndex idx, const DynInstPtr &new_inst)
{
    // sources[idx] = new_inst;
}

template <class DynInstPtr>
void
DependencyGraphReg<DynInstPtr>::clearInst(RegIndex idx)
{
    // DynInstPtr &inst = sources[idx];

    // for (int idx = 0; idx < numReservationStations; idx++) {
    //     if (inst == instructions[idx]) {
    //         instructions[idx] = NULL;
    //         break;
    //     }
    // }
}

template <class DynInstPtr>
bool
DependencyGraphReg<DynInstPtr>::empty(RegIndex reg_idx) const
{
    for (int inst_idx = 0; inst_idx < numReservationStations; inst_idx++) {
        if (0 != reservationStationMap[reg_idx][inst_idx]) {
            return false;
        }
    }

    return true;
}

template <class DynInstPtr>
void
DependencyGraphReg<DynInstPtr>::remove(RegIndex reg_idx,
                                    const DynInstPtr &inst_to_remove)
{
    int inst_idx = -1;

    for (int idx = 0; idx < numReservationStations; idx++) {
        if (instructions[idx] == inst_to_remove) {
            assert(-1 == inst_idx);
            inst_idx = idx;
        }
    }

    // if (-1 == inst_idx) {
    //     return;
    // }

    assert (-1 != inst_idx);

    reservationStationMap[reg_idx][inst_idx] = 0;
}

template <class DynInstPtr>
void
DependencyGraphReg<DynInstPtr>::retract(const DynInstPtr &inst)
{
    int inst_idx = -1;

    for (int idx = 0; idx < numReservationStations; idx++) {
        if (instructions[idx] == inst) {
            assert(-1 == inst_idx);
            inst_idx = idx;
        }
    }

    if (-1 == inst_idx) {
        return;
    }

    instructions[inst_idx] = NULL;
}

template <class DynInstPtr>
DynInstPtr
DependencyGraphReg<DynInstPtr>::pop(RegIndex reg_idx)
{
    DynInstPtr inst = NULL;

    for (int inst_idx = 0; inst_idx < numReservationStations; inst_idx++) {
        if (0 != reservationStationMap[reg_idx][inst_idx]) {
            inst = instructions[inst_idx];
            reservationStationMap[reg_idx][inst_idx]--;
            break;
        }
    }

    return inst;
}

template <class DynInstPtr>
bool
DependencyGraphReg<DynInstPtr>::empty() const
{
    for (int i = 0; i < numReservationStations; ++i) {
        if (!empty(i))
            return false;
    }
    return true;
}

template <class DynInstPtr>
void
DependencyGraphReg<DynInstPtr>::dump()
{
    cprintf("dump not implemented yet");

    // DepEntry *curr;

    // for (int i = 0; i < numEntries; ++i)
    // {
    //     curr = &dependGraph[i];

    //     if (curr->inst) {
    //         cprintf("dependGraph[%i]: producer: %s [sn:%lli] consumer: ",
    //                 i, curr->inst->pcState(), curr->inst->seqNum);
    //     } else {
    //         cprintf("dependGraph[%i]: No producer. consumer: ", i);
    //     }

    //     while (curr->next != NULL) {
    //         curr = curr->next;

    //         cprintf("%s [sn:%lli] ",
    //                 curr->inst->pcState(), curr->inst->seqNum);
    //     }

    //     cprintf("\n");
    // }
    // cprintf("memAllocCounter: %i\n", memAllocCounter);
}
// functions end

} // namespace o3
} // namespace gem5

#endif // __CPU_O3_DEP_GRAPH_REG_HH__
