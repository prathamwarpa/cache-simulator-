#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <queue>
#include <list>
#include <algorithm>
#include<math.h>

using namespace std;
char check ; 

enum Policy
{
    LRU,  FIFO,  OPT , LFU 
};

class Line
{
public:

    bool  valid;
       long long tag;
      long long bAddr;
     long long lru;      // last-used time  (LRU)
     long long ins;      // insertion time  (FIFO)
     long long freq ;  // (LFU)

    Line()
    {
        valid = false;
        tag  = 0 ;  bAddr = 0;
          lru = 0;
        ins = 0;
        freq = 0 ; 
    }

    Line(bool v, long long t, long long ba, long long l, long long i )
      {
        valid = v ;
         tag   = t;
         bAddr = ba;
         lru   = l;
         ins   = i;
         freq = 1 ; 
 }

};

class Set
{
public:

    vector<Line> ways;
    Set(int numWays)
    {
        ways.resize(numWays);
    }

    bool hit(long long tag, long long t)
    {
        for (auto& w : ways)
        {
            if (w.valid && w.tag == tag)
            {
                w.lru = t ;
                w.freq++ ; 
                return true;
            }
        }
        return false;
    }

    void insert(  long long tag,  long long  ba,  long long  t,   Policy  pol,  unordered_map<long long, queue<int>>&  nxt )
    {
        // 1. Use an empty way if one exists
        for (auto& w : ways)
        {
            if (!w.valid)
            {
                w = Line(true, tag, ba, t, t);
                return;
            }
        }

        int victim = 0;

       if ( pol == LFU) {
        long long freused = ways[0].freq ; 
        long long oldest = ways[0].lru ; 

        for(int i = 0 ; i < ways.size() ; i++){
            if ( freused > ways[i].freq ||  (freused == ways[i].freq && ways[i].lru < oldest)  )
            {
                freused = ways[i].freq ; 
                oldest = ways[i].lru ; 
                victim = i ; 
            }
        }
       }

       else  if (pol == LRU)
        {
            // Evict the least recently used way
            long long mintime = ways[0].lru;

            for (int i = 1; i <  ways.size(); i++)
            {
                if ( ways[i].lru < mintime)
                {
                    mintime = ways[i].lru;
                    victim  = i;
                }
            }
        }

        else if (pol == FIFO)
        {
            // Evict the way that was inserted first
            long long minins = ways[0].ins;

            for (int i = 1; i <  ways.size(); i++)
            {
                if (ways[i].ins < minins)
                {
                    minins = ways[i].ins;
                    victim = i;
                }
            }
        }
        else
        {
            // Belady / OPT  —  evict the way used farthest in the future
            int maxnext = -1;

            for (int i = 0; i < ways.size(); i++)
            {
                int nextUse = 1e9;          // assume never used again
                auto& q = nxt[ways[i].bAddr];

                while (!q.empty() && q.front() <= t)
                    q.pop();

                if (!q.empty())
                    nextUse = q.front();

                if (nextUse > maxnext)
                {
                    maxnext = nextUse;
                    victim  = i;
                }
            }
        }

        ways[victim] = Line(true, tag, ba, t, t);
    }

};
class Cache
{
public:

    int  sz;        // total cache size
    int  blk;       // block / line size
    int assoc;     // associativity
    int  nSets;     // number of sets
    Policy  pol;       // replacement policy

    vector<Set> sets;
    int    readh1 ,writeh1 ,  readm1 , writem1 ;      // L1 hits / misses
    int    readh2,writeh2 ,  readm2 , writem2 ;      // L2 hits / misses
    Cache* L2;          // pointer to next-level cache
    ofstream* outputFile; // OUTPUT FILE 
    

    Cache(int cacheSize, int blockSize, int associativity, Policy policy)
    {
        sz    = cacheSize;
        blk   = blockSize;
        assoc = associativity;
        pol   = policy;
        nSets = sz / (blk * assoc);
         readh1 = 0 ,writeh1 = 0 ,  readm1 = 0  , writem1 = 0  ;
        readh2 = 0 ,writeh2  = 0 ,  readm2 = 0  , writem2 = 0  ;
        L2 = nullptr;
        outputFile = NULL ; 
    
        for (int i = 0; i < nSets; i++)
        {
            sets.push_back(Set(assoc));
        }
    }
     
    void setOutputFile(ofstream* file)
    {
        outputFile = file;
    }

    void hookL2(Cache* next)
    {
        L2 = next;
    }

    bool chkHit(long long addr, long long t)
    {
        long long ba = addr / blk;
            return sets[ba % nSets].hit(ba / nSets, t);
    }


    // ── Bring a block into this cache ──
    void fill( long long  addr, long long  t, unordered_map<long long, queue<int>>&  nxt )
    {
        long long ba = addr / blk;
        sets[ba % nSets].insert(ba / nSets, ba, t, pol, nxt);
    }

    void access(long long addr , long long t , unordered_map<long long , queue<int>> & nxt)
    {    
        string hitmiss , loca , operation ; 
        (check == 'R' ? operation = "Read" : operation = "Write") ; 


        // L1 hit
        if (chkHit(addr, t))
          {
            (check == 'R' ? readh1++  : writeh1++) ; 
            hitmiss = "HIT" ; 
            loca = "L1" ; 

            if (outputFile && outputFile->is_open())
                *outputFile << hitmiss << " " << loca << " " << operation <<  "\n";
        
            return;
        }

        // L1 miss
        (check == 'R' ? readm1++ : writem1++) ; 

        if (L2 != nullptr)
        {
            // L2 hit
            if (L2->chkHit(addr, t)) {
                (check == 'R' ? readh2++ : writeh2++) ; 

                hitmiss = "MISS" ; 
                loca = "L2" ; 

                if (outputFile && outputFile->is_open())
                *outputFile << hitmiss << " " << loca << " " << operation <<  "\n";
        }
            // L2 miss  —  fetch from memory and fill L2
            else
            {
                 (check == 'R' ? readm2++ : writem2++) ; 
                 hitmiss = "MISS" ; 
                 loca = "RAM" ; 

                 if (outputFile && outputFile->is_open())
                *outputFile << hitmiss << " " << loca << " " << operation <<  "\n";
            
                 L2->fill(addr, t, nxt);
            }
        }
          else {
               hitmiss = "MISS" ; 
                 loca = "RAM" ; 
                 
                 if (outputFile && outputFile->is_open())
                *outputFile << hitmiss << " " << loca << " " << operation <<  "\n";
          }

        // Fill L1 regardless (data now in L2 or fetched from memory)
        fill(addr, t, nxt);
    }

    void print(const string& name)
    {
        int t1 = writeh1 + readh1 + writem1 + readm1 ;  
        int t2 = writeh2 + readh2 + writem2 + readm2 ; 

        cout << "\n";
        cout << "  [ " << name << " ]\n";
        cout << "\n";

        cout <<  "    L1  hits = " << " READS = " << readh1 << " WRITE = " << writeh1 << "\n" ;  
        cout <<  "   L1  misses = " << " READS = " << readm1 << " WRITE = " << writem1 << "\n"
             <<  "    hit rate = "
             << (t1 > 0 ? 100.0 * (writeh1 + readh1) / t1 : 0) << "%\n";

        if (L2 != nullptr)
        {
            cout << "\n";
               cout << "    L2  hits = " << " READS = " << readh2 << " WRITE = " << writeh2 << "\n" ;  
         cout <<  "   L2  misses = " << " READS = " << readm2 << " WRITE = " << writem2 << "\n"
             << "    hit rate = "
                 << ((writem1 + readm1) > 0 ? 100.0 * (writeh2 + readh2) / (writem1 + readm1) : 0) << "%\n";

               cout << "\n";
            cout << "    DRAM fetches  = " << (readm2 + writem2) << "\n";
             cout << "    Overall rate  = "
                 << (t1 > 0 ? 100.0 * (t1 - (readm2 + writem2)) / t1 : 0) << "%\n";
        }

         cout << "\n";

    }


void exportCSV(ofstream& csv, const string& policyName)
    {
        int l1hit = readh1 + writeh1;
        int l2hit = readh2 + writeh2; 
        int totalaccess = l1hit + readm1 + writem1; 
        int totalhit = l1hit + l2hit;
        int totalmiss = readm2 + writem2; 
        
        // Evictions = Misses - Cold Misses (Cache Capacity)
        int l1cap = sz / blk;
        int l2cap = L2 ? (L2->sz / L2->blk) : 0;

        int l1evictions = max(0, (readm1 + writem1) - l1cap);
        int l2evictions = max(0, (readm2 + writem2) - l2cap);
        int totalevictions = l1evictions + l2evictions;

        csv << policyName << "," << l1hit << "," << l2hit << "," << totalaccess << ","
            << totalhit << "," << totalevictions << "," << totalmiss << ","
            << l1evictions << "," << l2evictions << ","
            << readh1 << "," << readm1 << "," << writeh1 << "," << writem1 << ","
            << readh2 << "," << readm2 << "," << writeh2 << "," << writem2 << "\n";
    }

} ; 

class Req
{
public:

    char op;
    long long addr;

    Req(char operation, long long address)
    {
        op   = operation;
        addr = address;
    }
};

int main(int argc, char* argv[])
{
    // Default values
    int BLK = 64;
    int l1size = 1024, l1a = 2;
    int l2size = 8192, l2a = 4;

    
    if (argc >= 6) {
        BLK = stoi(argv[1]);
        l1size = stoi(argv[2]);
        l1a = stoi(argv[3]);
        l2size = stoi(argv[4]);
        l2a = stoi(argv[5]);
    }

    vector<Req> trace;
    unordered_map<long long, queue<int>> nxtUse;

    ifstream f("trace.txt");
    if (!f) 
        cerr << "Cannot open trace.txt\n";

    char op ; 
    string addrStr;
    int    idx = 0;
    
    while (f >> op >> addrStr)
    {
        long long addr = stoull(addrStr, nullptr, 16);
        trace.push_back(Req(op, addr));
        nxtUse[addr / BLK].push(idx++);
    }
    f.close();

    ofstream lruOut("lru_output.txt");
      ofstream fifoOut("fifo_output.txt");
     ofstream optOut("opt_output.txt");
    ofstream lfuOut("lfu_output.txt");

    
    Cache lruL1 (l1size,  BLK, l1a, LRU);
    Cache lruL2 (l2size,  BLK, l2a, LRU);

    Cache fifoL1(l1size,  BLK, l1a, FIFO);
    Cache fifoL2(l2size,  BLK, l2a, FIFO);

    Cache optL1 (l1size,  BLK, l1a, OPT);
    Cache optL2 (l2size,  BLK, l2a, OPT);

    Cache lfuL1 (l1size,  BLK, l1a, LFU);
    Cache lfuL2 (l2size,  BLK, l2a, LFU);

    lruL1 .hookL2(&lruL2);
      fifoL1.hookL2(&fifoL2);
    optL1 .hookL2(&optL2);
    lfuL1.hookL2(&lfuL2) ; 

    lruL1.setOutputFile(&lruOut);
    fifoL1.setOutputFile(&fifoOut);
    optL1.setOutputFile(&optOut);
    lfuL1.setOutputFile(&lfuOut);

    auto nxtCopy = nxtUse;

    for (int i = 0 ; i < trace.size() ; i++)
    {
        long long a = trace[i].addr;
         check = trace[i].op ; 
         lruL1.access(a ,i, nxtCopy);
         fifoL1.access(a, i, nxtCopy);
         optL1.access(a,i, nxtUse);
         lfuL1.access(a , i ,nxtCopy );
    }

    lruOut.close();
     fifoOut.close();
    optOut.close();
    lfuOut.close();

    lruL1.print("LRU");
    fifoL1.print("FIFO");
    optL1.print("OPTIMAL");
    lfuL1.print("LFU");

    ofstream csvOut("results.csv");

    csvOut << "policy,l1hit,l2hit,totalaccess,totalhit,totalevictions,totalmiss,"
           << "l1evictions,l2evictions,"
           << "readhitl1,readmissl1,writehitl1,writemissl1,"
           << "readhitl2,readmissl2,writehitl2,writemissl2\n";
           
    lruL1.exportCSV(csvOut, "LRU");
    fifoL1.exportCSV(csvOut, "FIFO");
    optL1.exportCSV(csvOut, "OPT");
    lfuL1.exportCSV(csvOut, "LFU");

    csvOut.close();
    return 0;
}