import path from 'path';
import { fileURLToPath } from 'url'; // Import fileURLToPath
import { expect } from 'chai';

// --- Workaround for Vert's BigInt JSON.stringify issue during logging ---
// Store original stringify
const originalStringify = JSON.stringify;
// Override JSON.stringify to handle BigInts by converting them to strings
JSON.stringify = function (value, replacer, space) {
    return originalStringify(value, (key, val) =>
        typeof val === 'bigint'
            ? val.toString() // Convert BigInt to string
            : val, // Return other values as is
    replacer, space);
};
// ------------------------------------------------------------------------

import vert from '@eosnetwork/vert';
// Import necessary components and helpers
// Assuming Name is needed for scopes or auth later
const { Blockchain, Name, nameToBigInt, expectToThrow, mintTokens } = vert;

// Get the directory name in ES module scope
const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

// --- Contract Paths ---
// Define paths consistently
const ORG_CONTRACT_PATH = path.join(__dirname, '../build/org/org');
const SUBSCRIPTION_CONTRACT_PATH = path.join(__dirname, '../build/subscription/subscription');
const EOSIO_TOKEN_BUILD_DIR = path.join(__dirname, '../build/eos-system-contracts/build/contracts/eosio.token'); // Path to the specific eosio.token contract build artifacts
const AUTHORITY_CONTRACT_PATH = path.join(__dirname, '../build/authority/authority'); // Added authority path

// --- Test Suite ---
describe('Org Contract Tests', () => {
    // Declare variables here to be accessible in tests
    let blockchain;
    let orgContract;
    let subscriptionContract;
    let tokenContract;
    let authorityContract; // Added authority contract variable
    let orgAdmin;
    let user1;

    // Helper: Define the token symbol (can be inside describe or outside)
    const EOS_SYMBOL = '4,EOS';

    beforeEach(async () => {
        // --- Fresh Blockchain State ---
        blockchain = new Blockchain();

        // --- Deploy Contracts ---
        // Deploy org contract with inline actions enabled
        orgContract = await blockchain.createContract(
            'org',
            ORG_CONTRACT_PATH, // Path prefix for wasm/abi
            { enableInline: true } // Enable eosio.code permission
        );

        // Deploy subscription contract
        subscriptionContract = await blockchain.createContract(
            'subscribedev',
            SUBSCRIPTION_CONTRACT_PATH // Path prefix for wasm/abi
        );

        // Deploy eosio.token contract manually
        const tokenWasm = blockchain.readWasm(path.join(EOSIO_TOKEN_BUILD_DIR, 'eosio.token.wasm'));
        const tokenAbi = blockchain.readAbi(path.join(EOSIO_TOKEN_BUILD_DIR, 'eosio.token.abi'));
        tokenContract = await blockchain.createAccount({
            name: 'eosio.token',
            wasm: tokenWasm, // Pass the promise
            abi: tokenAbi    // Pass the promise
        });
        console.log(`[DEBUG beforeEach] eosio.token created via createContract. typeof actions.create:`, typeof tokenContract?.actions?.create);
         // Optional check (can be removed if createContract reliably throws)
        if (!tokenContract || !tokenContract.actions) {
             throw new Error("eosio.token contract creation failed or actions unavailable.");
        }

        // Deploy authority contract
        authorityContract = await blockchain.createContract(
            'authority',
            AUTHORITY_CONTRACT_PATH // Path prefix for wasm/abi
        );
         if (!authorityContract || !authorityContract.actions) {
             throw new Error("authority contract creation failed or actions unavailable.");
         }

        // --- Create Accounts ---
        orgAdmin = await blockchain.createAccount('orgadmin1111');
        user1 = await blockchain.createAccount('user111111111');

        // --- Initial Configuration ---

        // TODO: Link Org contract to Authority contract
        // Replace 'setauthority' with the actual action name in your org contract
        // if (orgContract.actions.setauthority) {
        //    await orgContract.actions.setauthority([authorityContract.name]).send(`${orgContract.name}@active`);
        // } else {
        //    console.warn("Org contract does not have a 'setauthority' action. Linking skipped.");
        //    // Handle configuration via table if necessary
        // }

        // TODO: Grant initial permissions in Authority contract
        // Replace 'addpermission' and parameter structure with your authority contract's specifics
        // Example: Grant orgAdmin permission to call 'initorg' in the 'org' scope (or global scope '')
        // const orgScope = orgContract.name; // Or Name.from('') for global
        // const actionToPermit = Name.from('initorg');
        // await authorityContract.actions.addpermission([orgAdmin.name, orgScope, actionToPermit]).send(`${authorityContract.name}@active`);

        // Note: blockchain.resetTables() is usually not needed here as each test gets a fresh blockchain.
    });

    // --- Tests ---
    it('should allow an admin to register an organization using initorg', async () => {
        const orgName = 'testorg11111';
        const orgCode = 'test';
        const ipfsImage = 'ipfs://testhash';
        const orgDisplayName = 'Test Organization';
        const packageName = 'testpkg11111'; // Name for our test package

        // Define token parameters
        const tokenSymbol = 'EOS';
        const tokenPrecision = 4;
        const maxSupplyAmount = 1000000; // e.g., 1,000,000.0000 EOS
        const initialIssueAmount = 500000; // Issue half of max supply initially
        const transferAmount = 10; // Transfer 10.0000 EOS to orgAdmin

        const maxSupplyString = `${maxSupplyAmount.toFixed(tokenPrecision)} ${tokenSymbol}`;
        const initialIssueString = `${initialIssueAmount.toFixed(tokenPrecision)} ${tokenSymbol}`;
        const transferAmountString = `${transferAmount.toFixed(tokenPrecision)} ${tokenSymbol}`;

        // --- Setup Token and Fund Account using mintTokens helper ---
        // console.log('[DEBUG initorg] TOKEN_CONTRACT_PATH:', TOKEN_CONTRACT_PATH); // Path constant removed
        console.log('[DEBUG initorg] tokenContract.actions:', tokenContract.actions);
        console.log('[DEBUG initorg] typeof tokenContract.actions.create:', typeof tokenContract.actions.create);
        await mintTokens(tokenContract, tokenSymbol, tokenPrecision, maxSupplyAmount, transferAmount, [orgAdmin]);
        
        // --- Setup Subscription Package ---
        // Define cost for the package using the transfer amount for simplicity now
        // Use object format for extended_asset
        const costExtended = { 
            quantity: transferAmountString, // Asset string "10.0000 EOS"
            contract: tokenContract.name // Contract name "eosio.token"
        }; 
        
        // 1. Define the package type using `newpack` (called by contract owner)
        await subscriptionContract.actions.newpack([
            packageName,                 // package name
            'Test Package for Vert',     // descriptive_name
            1000n,                       // action_size (use BigInt 'n')
            3600n,                       // expiry_duration_in_secs (1 hour, use BigInt 'n')
            costExtended,                // cost (extended_asset object)
            true,                        // active
            true                         // display
        ]).send(`${subscriptionContract.name}@active`); // Authorized by the contract account itself

        // 2. Create the org account that will pay RAM etc.
        // This might implicitly require orgAdmin to have EOS if it pays RAM
        const orgAccount = await blockchain.createAccount(orgName);

        // 3. Assign the package by simulating a transfer to trigger `buypack`
        //    (orgAdmin pays, memo specifies org and package)
        //    Use the same amount as the cost for simplicity
        const memo = `${orgName}:${packageName}`;
        await tokenContract.actions.transfer([
            orgAdmin.name,               // from
            subscriptionContract.name,   // to
            transferAmountString,        // quantity (asset string "10.0000 EOS")
            memo                         // memo
        ]).send(`${orgAdmin.name}@active`);

        // --- Call the 'initorg' action ---
        // Authorization: orgAdmin initiates, orgAccount pays RAM/signs implicitly too?
        // Let's try with both authorizations required by original code
        await orgContract.actions.initorg(
            [orgName, orgCode, ipfsImage, orgDisplayName]
        ).send([`${orgName}@active`, `${orgAdmin.name}@active`]); // Try providing both org and admin auth

        // --- Assertions ---
        const orgTable = orgContract.tables.orgs(nameToBigInt(orgContract.name.toString())); // Scope via nameToBigInt
        const orgRow = await orgTable.get(nameToBigInt(orgName)); // Primary key via nameToBigInt

        expect(orgRow).not.to.be.undefined;
        if (orgRow) { // Type guard for stricter checking
             expect(orgRow.org.toString()).to.equal(orgName); // Compare string representation
             expect(orgRow.org_code.toString()).to.equal(orgCode);
             // TODO: Add check for initial member (orgAdmin?) if applicable
             // const membersTable = orgContract.tables.members(nameToBigInt(orgName)); // Scope is org name
             // const adminMember = await membersTable.get(nameToBigInt(orgAdmin.name));
             // expect(adminMember).not.to.be.undefined;
        }    });

    // ... other tests ...

    // Helper function to create a standard org for tests
    // Avoids repeating the full token/subscription setup in every test
    async function createTestOrg(orgName = 'testorg11111', admin = orgAdmin) {
        const orgCode = 'test';
        const ipfsImage = 'ipfs://testimage';
        const orgDisplayName = 'Test Org for Members';
        const packageName = 'testpkgmem11'; // Use unique package name per call if needed

        // Minimal token setup just for this org
        const tokenSymbol = 'EOS';
        const tokenPrecision = 4;
        const transferAmount = 1; // Minimal amount needed if any
        const transferAmountString = `${transferAmount.toFixed(tokenPrecision)} ${tokenSymbol}`;
        // console.log('[DEBUG createTestOrg] TOKEN_CONTRACT_PATH:', TOKEN_CONTRACT_PATH); // Path constant removed
        console.log('[DEBUG createTestOrg] tokenContract.actions:', tokenContract.actions);
        console.log('[DEBUG createTestOrg] typeof tokenContract.actions.create:', typeof tokenContract.actions.create);
        // --- Replace mintTokens with direct create/issue calls ---
        const maxSupplyString = `${(1000).toFixed(tokenPrecision)} ${tokenSymbol}`;
        const issueAmountString = `${transferAmount.toFixed(tokenPrecision)} ${tokenSymbol}`;

        // 1. Create the token
        console.log(`[DEBUG createTestOrg - create] issuer: ${admin.name} (${typeof admin.name}), maxSupply: ${maxSupplyString} (${typeof maxSupplyString})`);
        console.log(`[DEBUG createTestOrg - create] auth: ${tokenContract.name}@active`);
        await tokenContract.actions.create(
            [admin.name, maxSupplyString]
        ).send(`${tokenContract.name}@active`); // Authority of eosio.token contract

        // 2. Issue tokens to the admin account
        console.log(`[DEBUG createTestOrg - issue] to: ${admin.name} (${typeof admin.name}), quantity: ${issueAmountString} (${typeof issueAmountString}), memo: 'Initial issue for test'`);
        console.log(`[DEBUG createTestOrg - issue] auth: ${admin.name}@active`);
        await tokenContract.actions.issue(
            [admin.name, issueAmountString, 'Initial issue for test']
        ).send(`${admin.name}@active`); // Authority of the issuer (admin)
        // -----------------------------------------------------------


        // Minimal subscription setup
         const costExtended = { quantity: transferAmountString, contract: tokenContract.name };
         try { // Use try-catch in case package already exists from previous failed test run if not using beforeEach reset
             await subscriptionContract.actions.newpack([
                 packageName, 'Test Pkg Members', 1000n, 3600n, costExtended, true, true
             ]).send(`${subscriptionContract.name}@active`);
         } catch (e) {
             // Ignore if package already exists error
             if (!e.message.includes('Package already exists')) throw e;
         }

        // Create the org account if it doesn't exist (name might be reused)
        let orgAccount;
        try {
            orgAccount = await blockchain.createAccount(orgName);
        } catch(e) {
            if (e.message.includes('Account name already exists')) {
                orgAccount = blockchain.getAccount(orgName);
            } else {
                throw e;
            }
        }


        // Assign package via transfer
        const memo = `${orgName}:${packageName}`;
        await tokenContract.actions.transfer([
            admin.name, subscriptionContract.name, transferAmountString, memo
        ]).send(`${admin.name}@active`);

        // Initialize the org - Mirror auth from first test
        await orgContract.actions.initorg(
            [orgName, orgCode, ipfsImage, orgDisplayName]
        ).send([`${orgName}@active`, `${admin.name}@active`]);

        return orgAccount; // Return the org account object if needed elsewhere
    }

    it('should allow an org admin to add a member', async () => {
        const orgName = 'addmemtest11';
        await createTestOrg(orgName, orgAdmin); // Create the org first

        // Add user1 as a member, authorized by orgAdmin
        await orgContract.actions.addmember([orgName, user1.name]).send(`${orgAdmin.name}@active`);

        // Verify member exists
        const membersTable = orgContract.tables.members(nameToBigInt(orgName)); // Scope via nameToBigInt
        const memberRow = await membersTable.get(nameToBigInt(user1.name.toString())); // Primary key via nameToBigInt

        expect(memberRow).not.to.be.undefined;
        if (memberRow) {
            expect(memberRow.account).to.equal(user1.name.toString());
            // Add checks for other fields like role if applicable
        }
    });

    it('should prevent a non-admin from adding a member', async () => {
        const orgName = 'addfailtest1';
        await createTestOrg(orgName, orgAdmin); // orgAdmin creates the org

        // Create another user
        const user2 = await blockchain.createAccount('user211111111');

        // Attempt to add user2 by user1 (who is not an admin of this org)
        await expectToThrow(
            orgContract.actions.addmember([orgName, user2.name]).send(`${user1.name}@active`),
            /authority check failed|permission check failed/ // Adjust regex based on actual error from authority contract
        );

        // Verify user2 was not added
        const membersTable = orgContract.tables.members(nameToBigInt(orgName));
        const memberRow = await membersTable.get(nameToBigInt(user2.name.toString()));
        expect(memberRow).to.be.undefined;
    });

    it('should allow an org admin to remove a member', async () => {
        const orgName = 'remmemtest';
        await createTestOrg(orgName, orgAdmin);

        // Add user1 first
        await orgContract.actions.addmember([orgName, user1.name]).send(`${orgAdmin.name}@active`);

        // Verify user1 was added
        const membersTable = orgContract.tables.members(nameToBigInt(orgName));
        let memberRow = await membersTable.get(nameToBigInt(user1.name.toString()));
        expect(memberRow).not.to.be.undefined;

        // Remove user1, authorized by orgAdmin
        await orgContract.actions.removemember([orgName, user1.name]).send(`${orgAdmin.name}@active`);

        // Verify user1 was removed
        memberRow = await membersTable.get(nameToBigInt(user1.name.toString()));
        expect(memberRow).to.be.undefined;
    });

     it('should prevent a non-admin from removing a member', async () => {
        const orgName = 'remfailtest';
        await createTestOrg(orgName, orgAdmin);

        // Add user1 first (by orgAdmin)
        await orgContract.actions.addmember([orgName, user1.name]).send(`${orgAdmin.name}@active`);

         // Create another user
        const user2 = await blockchain.createAccount('user211111111');

        // Attempt to remove user1 by user2
        await expectToThrow(
            orgContract.actions.removemember([orgName, user1.name]).send(`${user2.name}@active`),
             /authority check failed|permission check failed/ // Adjust regex
        );

        // Verify user1 still exists
        const membersTable = orgContract.tables.members(nameToBigInt(orgName));
        const memberRow = await membersTable.get(nameToBigInt(user1.name.toString()));
        expect(memberRow).not.to.be.undefined;
    });
});
