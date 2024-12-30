// Project Identifier: 0E04A31E0D60C01986ACB20081C9D8722A1899B6
#include <climits>
#include <functional>
#include <iostream>
#include <queue>
#include <tuple>
#include <string>
#include <getopt.h>
#include <sstream>
#include "P2random.h"
#include <utility>
#include <vector>
using namespace std;


//make command line enums
//verbose
enum class VerboseMode
{
Inactive = 0,
Active,
};
//median
enum class MedianMode
{
Inactive = 0,
Active,
};
//tradeDetails
enum class TradeDetailsMode
{
Inactive = 0,
Active,
};
//timeTravelers
enum class TimeTravel
{
Inactive = 0,
Active,
};

struct Options 
{ 
  //will convert these options from command line 
//into variables in main
VerboseMode verboseMode = VerboseMode::Inactive;
MedianMode medianMode = MedianMode::Inactive;
TradeDetailsMode tradeDetailsMode = TradeDetailsMode::Inactive;
TimeTravel timeTravel = TimeTravel::Inactive;
}; 


/*steps: (from spec)
1. Print Startup Output (done)
2. Read the next order from input 
3. If the new order’s TIMESTAMP != CURRENT_TIMESTAMP 
    a. If the --median/-m option is specified, 
    print the median price of all stocks 
    that have been traded on at least once 
    by this point in ascending order 
    by stock ID (Median Output) (done)
    b. Set CURRENT_TIMESTAMP to be the new order’s TIMESTAMP. (done)
4. Add the new order to the market (done)
5. Make all possible matches in the market that now includes the new order 
(observing the fact that a single incoming order may make matches with multiple existing orders) 
    a. If a match is made and the --verbose/-v option is specified, print Verbose Output 
    b. Remove any completed orders from the market 
    c. Update the share quantity of any partially fulfilled order in the market
6. Repeat steps 2-5 until there are no more orders
7. If the --median/-m flag is set, output final timestamp median information of all stocks that were traded that day (Median Output)
8. Print Summary Output
9. If the --trader_info/-i flag is set print Trader Info Output
10. If the --time_travelers/-t flag is set print Time Travelers Output
*/
struct Order
{
Order(bool buyOrder, int32_t traderID, 
int32_t limit, int32_t quantity, int32_t lineNum);
bool buyOrder;
int32_t traderID;
int32_t limit;
int32_t quantity;
int32_t lineNum;
};

Order::Order(bool buyOrder, 
int32_t traderID, int32_t limit, int32_t quantity, int32_t lineNum)
:buyOrder(buyOrder),traderID(traderID),
limit(limit),quantity(quantity),lineNum(lineNum){}

struct SellOrderComp
{
bool operator()(const Order& a, const Order& b)
{
if(a.limit != b.limit)
{
return a.limit > b.limit;
}
return a.lineNum > b.lineNum;
}
};

struct BuyOrderComp
{
bool operator()(const Order& a, const Order& b)
{
if(a.limit != b.limit)
{
return a.limit < b.limit;
}
return a.lineNum > b.lineNum;
}
};

class Median
{
private:
priority_queue<int32_t> leftValues;
priority_queue<int32_t,vector<int32_t>, greater<>> rightValues;

public:
void addNumber(int32_t num) {
        if (leftValues.empty() || num <= leftValues.top()) {
            leftValues.push(num);
        } else {
            rightValues.push(num);
        }

        if (leftValues.size() > rightValues.size() + 1) {
            rightValues.push(leftValues.top());
            leftValues.pop();
        } else if (rightValues.size() > leftValues.size()) {
            leftValues.push(rightValues.top());
            rightValues.pop();
        }
    }


int32_t getMedian()
{
if(leftValues.size() != rightValues.size())
{
return leftValues.top();
}
const int32_t median = (leftValues.top() + rightValues.top()) / 2;
return median;
}
};


struct TimeTravelerData {
    pair<int32_t,int32_t> minSellOrder;    // Order with the minimum sell price so far
    pair<int32_t,int32_t> bestBuyOrder;    // Buy order that maximizes profit
    pair<int32_t,int32_t> bestSellOrder;   // Corresponding sell order for that buy
    int32_t maxProfit;
    TimeTravelerData();
};

TimeTravelerData::TimeTravelerData()
:minSellOrder(-1,INT_MAX),bestBuyOrder(-1,0),bestSellOrder(-1,0),maxProfit(INT_MIN){}


struct Stock
{
bool tradedOn = false;
TimeTravelerData timeTravelInfo;
Median med = Median();
priority_queue<Order,vector<Order>,SellOrderComp> sells = 
priority_queue<Order,vector<Order>,SellOrderComp>(); 
priority_queue<Order,vector<Order>,BuyOrderComp> buys = 
priority_queue<Order,vector<Order>,BuyOrderComp>();
};

struct Trader
{
    int32_t bought;
    int32_t sold;
    int32_t net;
    Trader();
};
Trader::Trader()
:bought(0),sold(0),net(0){}

class Market
{
private:
bool verbose;
bool median;
bool details;
bool timeTravel;
int32_t numTrades = 0;
int32_t numTraders = 0;
int32_t numStocks = 0;
int32_t randomSeed = 0;
int32_t numOrders = 0;
int32_t arrivalRate = 0;
vector<Trader> traderInfo;
vector<Stock> stocks;
int32_t currentTimestamp = 0;
stringstream ss;
string mode; //either TL for trade list or PR for pesudorandom
void process(istream &is);
void printMedians();
void processBuyTrade(Order& o, int32_t currentStockID, int32_t currentTimestamp);
void processSellTrade(Order& o, int32_t currentStockID, int32_t currentTimestamp);
void printTradeDetails();
void printTimeTravelDetails();
public:
Market(bool verbose, bool median, bool details, bool timeTravel);
void setDetails();
void processOrders();
void printOutputs();
};

Market::Market(bool verbose, bool median, bool details, bool timeTravel)
: verbose(verbose), median(median), details(details), timeTravel(timeTravel){}

void Market::setDetails()
{
    string junkString;
    getline(cin, junkString);
    char listMode;
    cin >> junkString >> listMode;
    mode = (listMode == 'T') ? "TL":"PR";
    getline(cin,junkString);
    string traderNum;
    cin >> junkString >> traderNum;
    numTraders = stoi(traderNum);
    string stockNum;
    cin >> junkString >> stockNum;
    numStocks = stoi(stockNum);
    const auto nt = static_cast<uint32_t>(numTraders);
    const auto ns = static_cast<uint32_t>(numStocks);
    traderInfo.resize(nt,Trader());
    stocks.resize(ns,Stock());
    if(mode == "PR")
    {
    cin >> junkString >> randomSeed;
    cin >> junkString >> numOrders;
    cin >> junkString >> arrivalRate;
    const auto rs = static_cast<uint32_t>(randomSeed);
    const auto no = static_cast<uint32_t>(numOrders);
    const auto ar = static_cast<uint32_t>(arrivalRate);
    P2random::PR_init(ss, 
    rs, nt, 
    ns, no, ar);
    }
}

void Market::processOrders()
{
    if (mode == "PR") 
    {
    process(ss);
    }
    else 
    {
    process(cin);
    }
}

void Market::process(istream &is)
{
    int lineNum = 0;
    int newTimestamp;
    int currentTraderID;
    int currentStockID;
    int price;
    int quantity;
    char junkChar;
    string buyOrSell;
    while(is >> newTimestamp >> buyOrSell >> junkChar >> currentTraderID >> junkChar
    >> currentStockID>> junkChar >> price >> junkChar
    >> quantity)
    {
        if(newTimestamp < 0)
    {
        cerr << "Error: Negative timestamp\n";
        exit(1);
    }
    if(newTimestamp < currentTimestamp)
    {
        cerr << "Error: Decreasing timestamp\n";
        exit(1);
    }
    if(currentTraderID < 0 || currentTraderID >= numTraders)
    {
        cerr << "Error: Invalid trader ID\n";
        exit(1);
    }
    if(currentStockID < 0 || currentStockID >= numStocks)
    {
    cerr << "Error: Invalid stock ID\n";
     exit(1);
    }
    if(price <= 0)
    {
        cerr << "Error: Invalid price\n";
        exit(1);
    }
    if(quantity <= 0)
    {
        cerr << "Error: Invalid quantity\n";
        exit(1);
    }
    const bool orderType = (buyOrSell == "BUY");
    Order order(orderType,currentTraderID,price,quantity,lineNum);
    lineNum++;
    if(newTimestamp != currentTimestamp)
    {
    if(median)
    {
    printMedians();
    }
    currentTimestamp = newTimestamp;
    }
    if(orderType)
    {
            processBuyTrade(order,currentStockID,currentTimestamp);
    }
    else
    {
            processSellTrade(order,currentStockID,currentTimestamp);
    }
    }
}

void Market::processBuyTrade(Order& o, int32_t currentStockID, int32_t currentTimestamp) {
    const auto sID = static_cast<uint32_t>(currentStockID);
    if(stocks[sID].timeTravelInfo.minSellOrder.first != -1)
    {
        int32_t potentialProfit = o.limit-stocks[sID].timeTravelInfo.minSellOrder.second;
        if (potentialProfit > stocks[sID].timeTravelInfo.maxProfit || 
            (potentialProfit == stocks[sID].timeTravelInfo.maxProfit && stocks[sID].timeTravelInfo.minSellOrder.first 
            < stocks[sID].timeTravelInfo.bestSellOrder.first) || 
            (potentialProfit == stocks[sID].timeTravelInfo.maxProfit && stocks[sID].timeTravelInfo.minSellOrder.first 
            == stocks[sID].timeTravelInfo.bestSellOrder.first && 
             currentTimestamp < stocks[sID].timeTravelInfo.bestBuyOrder.first)) 
             {
            stocks[sID].timeTravelInfo.maxProfit = potentialProfit;
            stocks[sID].timeTravelInfo.bestBuyOrder.first = currentTimestamp;
            stocks[sID].timeTravelInfo.bestBuyOrder.second = o.limit;
            stocks[sID].timeTravelInfo.bestSellOrder.first = stocks[sID].timeTravelInfo.minSellOrder.first;
            stocks[sID].timeTravelInfo.bestSellOrder.second = stocks[sID].timeTravelInfo.minSellOrder.second;
        }

    }
    // Process while there's quantity to buy and matching sell orders
    while (o.quantity > 0 && !stocks[sID].sells.empty()) {
        Order bestSell = stocks[sID].sells.top();
        // Check if the sell offer matches the buy offer price
        if (bestSell.limit > o.limit) {
            break;
        }

        // Determine transaction details
        int32_t transactionPrice = bestSell.limit;
        int32_t sharesTraded = std::min(o.quantity, bestSell.quantity);

        // Execute the trade
        const auto buyerID = static_cast<uint32_t>(o.traderID);
        const auto sellerID = static_cast<uint32_t>(bestSell.traderID);

        traderInfo[buyerID].bought += sharesTraded;
        traderInfo[sellerID].sold += sharesTraded;
        traderInfo[buyerID].net -= transactionPrice * sharesTraded;
        traderInfo[sellerID].net += transactionPrice * sharesTraded;

        // Update the median price
        stocks[sID].med.addNumber(transactionPrice);
        stocks[sID].tradedOn = true;
        numTrades++;

        // Display the transaction
        if (verbose) {
            std::cout << "Trader " << o.traderID << " purchased " << sharesTraded 
                      << " shares of Stock " << currentStockID << " from Trader " << bestSell.traderID 
                      << " for $" << transactionPrice << "/share\n";
        }

        // Update quantities after the transaction
        o.quantity -= sharesTraded;
        bestSell.quantity -= sharesTraded;
        
        // Remove the sell order from the queue and reinsert if there's remaining quantity
        stocks[sID].sells.pop();
        if (bestSell.quantity > 0) {
            stocks[sID].sells.push(bestSell);
        }
}
if(o.quantity > 0)
{
    stocks[sID].buys.push(o);
}
}

void Market::processSellTrade(Order& o, int32_t currentStockID, int32_t currentTimestamp) {
    const auto sID = static_cast<uint32_t>(currentStockID);

    if(o.limit < stocks[sID].timeTravelInfo.minSellOrder.second || 
    (o.limit == stocks[sID].timeTravelInfo.minSellOrder.second && 
    currentTimestamp < stocks[sID].timeTravelInfo.minSellOrder.first))
        {
            stocks[sID].timeTravelInfo.minSellOrder.first = currentTimestamp;
            stocks[sID].timeTravelInfo.minSellOrder.second = o.limit;
        }
    // Process while there's quantity to sell and matching buy orders
    while (o.quantity > 0 && !stocks[sID].buys.empty()) {
        Order bestBuy = stocks[sID].buys.top();

        // Check if the buy offer matches the sell offer price
        if (bestBuy.limit < o.limit) {
            break;
        }
        // Determine transaction details
        int32_t transactionPrice = bestBuy.limit;
        int32_t sharesTraded = std::min(o.quantity, bestBuy.quantity);

        // Execute the trade
        const auto sellerID = static_cast<uint32_t>(o.traderID);
        const auto buyerID = static_cast<uint32_t>(bestBuy.traderID);

        traderInfo[sellerID].sold += sharesTraded;
        traderInfo[buyerID].bought += sharesTraded;
        traderInfo[sellerID].net += transactionPrice * sharesTraded;
        traderInfo[buyerID].net -= transactionPrice * sharesTraded;

        // Update the median price
        stocks[sID].med.addNumber(transactionPrice);
        stocks[sID].tradedOn = true;
        numTrades++;

        // Display the transaction
        if (verbose) {
        cout << "Trader " << bestBuy.traderID << " purchased " << sharesTraded 
            << " shares of Stock " << currentStockID << " from Trader " << o.traderID 
            << " for $" << transactionPrice << "/share\n";
        }

        // Update quantities after the transaction
        o.quantity -= sharesTraded;
        bestBuy.quantity -= sharesTraded;

        // Remove the buy order from the queue and reinsert if there's remaining quantity
        stocks[sID].buys.pop();
        if (bestBuy.quantity > 0) {
            stocks[sID].buys.push(bestBuy);
        }
    }
    if(o.quantity > 0)
    {
        stocks[sID].sells.push(o);
    }
    }

    // If there are remaining shares, push the remaining order into the sell queue


void Market::printMedians()
{
for(size_t i = 0; i<stocks.size(); i++)
{
if (stocks[i].tradedOn)
{
     
cout << "Median match price of Stock " << i << " at time " << currentTimestamp 
<< " is $" << stocks[i].med.getMedian() << "\n";
}
}
}

void Market::printOutputs()
{
if(median)
{
printMedians();
}
cout << "---End of Day---\n";
cout << "Trades Completed: " << numTrades << "\n";
if(details)
{
printTradeDetails();
}
if(timeTravel)
{
printTimeTravelDetails();
}
}

void Market::printTradeDetails()
{
cout << "---Trader Info---\n";
    for(size_t i =0; i < traderInfo.size(); i++)
    {
        cout << "Trader " << i << " bought " << traderInfo[i].bought << " and sold " <<
        traderInfo[i].sold << " for a net transfer of $" << traderInfo[i].net << "\n";
    }
}

void Market::printTimeTravelDetails()
{
cout << "---Time Travelers---\n";
for(size_t i =0; i < stocks.size(); i++)
    {
        if(stocks[i].timeTravelInfo.bestBuyOrder.first == -1 || stocks[i].timeTravelInfo.bestSellOrder.first == -1
        || stocks[i].timeTravelInfo.maxProfit < 0)
        {
            cout << "A time traveler could not make a profit on Stock " << i << "\n";
        }
        else 
        {
        cout << "A time traveler would buy Stock " << i << " at time " << stocks[i].timeTravelInfo.bestSellOrder.first << 
        " for $" << stocks[i].timeTravelInfo.bestSellOrder.second << " and sell it at time " << stocks[i].timeTravelInfo.bestBuyOrder.first
        << " for $" << stocks[i].timeTravelInfo.bestBuyOrder.second << "\n";
        }
    }
}

void setModes(int argc, char * argv[], Options &options) 
{
  // uses getopt_long() to parse command line for modes as well as error checking
  opterr = false;
  int choice;
  int index = 0;
  option long_options[] = {
    {"verbose", no_argument,        nullptr,        'v'},
    {"median", no_argument,        nullptr,         'm'},
    {"trader_info", no_argument, nullptr,           'i'},
    {"time_travelers",   no_argument,      nullptr, 't'},
    { nullptr, 0, nullptr, '\0' }
  };


  while ((choice = getopt_long(argc, argv, "vmit", long_options, &index)) != -1) {
    switch (choice) {
      case 'v':
        options.verboseMode = VerboseMode::Active;
        break;

      case 'm':
        options.medianMode = MedianMode::Active;
        break;
    
     case 'i':
        options.tradeDetailsMode = TradeDetailsMode::Active;
        break;

     case 't':
        options.timeTravel = TimeTravel::Active;
        break;

    default:
        cerr << "Error: Unknown command line option\n";
        exit(1);
    }  // switch ..choice
  }  // while
} 

int main(int argc, char* argv[])
{
ios_base::sync_with_stdio(false);
Options options;
setModes(argc,argv,options);
bool verbose = false;
bool median = false;
bool details = false;
bool timeTravel = false;
if(options.verboseMode == VerboseMode::Active)
{
    verbose = true;
}
if(options.medianMode == MedianMode::Active)
{
    median = true;
}
if(options.tradeDetailsMode == TradeDetailsMode::Active)
{
    details = true;
}
if(options.timeTravel == TimeTravel::Active)
{
    timeTravel = true;
}
Market market(verbose,median,details,timeTravel);
cout << "Processing orders...\n";
market.setDetails();
market.processOrders();
market.printOutputs();
}
