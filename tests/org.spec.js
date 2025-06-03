import path from 'path';
import { fileURLToPath } from 'url'; // Import fileURLToPath
import chai, { expect } from 'chai';
import chaiAsPromised from 'chai-as-promised';
chai.use(chaiAsPromised);

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
import { Asset } from '@greymass/eosio';
import { symbolCodeToBigInt } from '@eosnetwork/vert/dist/antelope/bn.js';
// Import necessary components and helpers
// Assuming Name is needed for scopes or auth later
const { Blockchain, Name, nameToBigInt, expectToThrow, mintTokens } = vert;

// Get the directory name in ES module scope
const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

// --- Contract Paths ---
// Define paths consistently
const ORG_CONTRACT_NAME = 'org';
const AUTH_CONTRACT_NAME = 'authority';
const SUB_CONTRACT_NAME = 'subscription';
const BADGE_CONTRACT_NAME = 'badgedata';
const EMITTER_CONTRACT_NAME = 'andemitter';
const TOKEN_CONTRACT_NAME = 'eosio.token'; // Assuming standard token contract for potential future use

// Define account names (ensure they are 12 characters)
const ORG_ACCOUNT_NAME = 'testorganiza'; // Org contract account
const AUTH_ACCOUNT_NAME = 'testauthorit'; // Authority contract account
const SUB_ACCOUNT_NAME = 'subscribedev'; // Subscription contract account (Match C++ macro for test)
const BADGE_ACCOUNT_NAME = 'testbadgedat'; // Badge data contract account
const EMITTER_ACCOUNT_NAME = 'testemitters'; // Emitter contract account
const ALICE_ACCOUNT_NAME = 'aliceaccount';
const BOB_ACCOUNT_NAME = 'bobaccount';
const TOKEN_ACCOUNT_NAME = 'eosio.token';  // Standard token contract account name

// Define contract build DIRECTORY paths
const ORG_CONTRACT_BUILD_DIR = path.join(__dirname, '../build/org');
const AUTH_CONTRACT_BUILD_DIR = path.join(__dirname, '../build/authority');
const SUB_CONTRACT_BUILD_DIR = path.join(__dirname, '../build/subscription');
const BADGE_CONTRACT_BUILD_DIR = path.join(__dirname, '../build/badgedata');
const EMITTER_CONTRACT_BUILD_DIR = path.join(__dirname, '../build/andemitter');
const EOSIO_TOKEN_BUILD_DIR = path.join(__dirname, '../build/eos-system-contracts/build/contracts/eosio.token'); // Path to the specific eosio.token contract build artifacts

// --- Test Suite ---
describe('Org Contract Tests', () => {
    beforeEach(() => {
        if (blockchain && typeof blockchain.resetTables === 'function') {
            blockchain.resetTables();
        }
    });
    // Declare variables here to be accessible in tests
    let blockchain;
    let orgContract;
    let subscriptionContract;
    let tokenContract;
    let authorityContract;
    let badgeContract;
    let emitterContract;
    let orgAdmin;
    let user1;

    // Helper: Define the token symbol (can be inside describe or outside)
    const EOS_SYMBOL = 'EOS';

    // --- Setup: Runs before each test in the suite ---
    beforeEach(async () => {
        // --- Blockchain Setup ---
        blockchain = new Blockchain(); // Fresh blockchain for each test

        // --- Account Creation ---
        // Create necessary user and contract accounts
        const accounts = [ORG_ACCOUNT_NAME, ALICE_ACCOUNT_NAME, BOB_ACCOUNT_NAME, AUTH_ACCOUNT_NAME, SUB_ACCOUNT_NAME, BADGE_ACCOUNT_NAME, EMITTER_ACCOUNT_NAME, TOKEN_ACCOUNT_NAME];
        blockchain.createAccounts(...accounts);

        // --- Contract Deployment (Refactored for Synchronous Action Building) ---
        // 1. Read all WASM/ABI files first
        const [ 
            orgWasm, orgAbi,
            authWasm, authAbi,
            subWasm, subAbi,
            badgeWasm, badgeAbi,
            emitterWasm, emitterAbi,
            tokenWasm, tokenAbi // Add token WASM/ABI variables
        ] = await Promise.all([
            blockchain.readWasm(path.join(ORG_CONTRACT_BUILD_DIR, 'org.wasm')),
            blockchain.readAbi(path.join(ORG_CONTRACT_BUILD_DIR, 'org.abi')),
            blockchain.readWasm(path.join(AUTH_CONTRACT_BUILD_DIR, 'authority.wasm')),
            blockchain.readAbi(path.join(AUTH_CONTRACT_BUILD_DIR, 'authority.abi')),
            blockchain.readWasm(path.join(SUB_CONTRACT_BUILD_DIR, 'subscription.wasm')),
            blockchain.readAbi(path.join(SUB_CONTRACT_BUILD_DIR, 'subscription.abi')),
            blockchain.readWasm(path.join(BADGE_CONTRACT_BUILD_DIR, 'badgedata.wasm')),
            blockchain.readAbi(path.join(BADGE_CONTRACT_BUILD_DIR, 'badgedata.abi')),
            blockchain.readWasm(path.join(EMITTER_CONTRACT_BUILD_DIR, 'andemitter.wasm')),
            blockchain.readAbi(path.join(EMITTER_CONTRACT_BUILD_DIR, 'andemitter.abi')),
            // Read eosio.token WASM and ABI
            blockchain.readWasm(path.join(EOSIO_TOKEN_BUILD_DIR, 'eosio.token.wasm')),
            blockchain.readAbi(path.join(EOSIO_TOKEN_BUILD_DIR, 'eosio.token.abi')),
        ]);

        // 2. Create accounts with resolved WASM/ABI
        [orgContract, authorityContract, subscriptionContract, badgeContract, emitterContract, tokenContract] = await Promise.all([
            blockchain.createAccount({ name: ORG_ACCOUNT_NAME, wasm: orgWasm, abi: orgAbi, enableInline: true }),
            blockchain.createAccount({ name: AUTH_ACCOUNT_NAME, wasm: authWasm, abi: authAbi, enableInline: true }),
            blockchain.createAccount({ name: SUB_ACCOUNT_NAME, wasm: subWasm, abi: subAbi, enableInline: true }),
            blockchain.createAccount({ name: BADGE_ACCOUNT_NAME, wasm: badgeWasm, abi: badgeAbi, enableInline: true }),
            blockchain.createAccount({ name: EMITTER_ACCOUNT_NAME, wasm: emitterWasm, abi: emitterAbi, enableInline: true }),
            // Deploy eosio.token contract
            blockchain.createAccount({ name: TOKEN_ACCOUNT_NAME, wasm: tokenWasm, abi: tokenAbi, enableInline: true })
        ]);

        // Await contract readiness (optional, but good practice if experiencing timing issues)
        // Note: With the refactor above, this explicit await might no longer be strictly necessary
        // as actions should be built synchronously, but leaving it doesn't hurt.
        await Promise.all([orgContract, authorityContract, subscriptionContract, badgeContract, emitterContract, tokenContract].map(c => c.vm?.ready));

        // --- Set eosio.code Permissions (Crucial for Inline Actions) ---
        // This is handled automatically by createAccount with enableInline=true

        // --- Contract Initialization (If necessary) ---
        // Call any required 'init' actions on deployed contracts.
        // e.g., await blockchain.contracts[AUTH_CONTRACT_NAME].actions.init([]).send();

        // --- Assign Admin Account --- 
        // Assign orgAdmin to the org contract account itself, as intended
        orgAdmin = blockchain.getAccount(ORG_ACCOUNT_NAME);

        user1 = await blockchain.createAccount('user111111111');

        // --- Define Helper Function INSIDE beforeEach ---
        async function createTestOrg(orgName = ORG_ACCOUNT_NAME, admin = orgAdmin) {
            console.log(`[createTestOrg] Starting for org: ${orgName}, admin: ${admin.name}`);
            const orgCode = 'test'; // Always 4 lowercase letters
            const ipfsImage = 'ipfs://testimage';
            const orgDisplayName = 'Test Org for Members';
            const packageName = `pkg${orgName.substring(0, 8)}`; // Create unique package name based on orgName

            // Minimal token setup just for this org
            const tokenSymbol = 'EOS';
            const tokenPrecision = 4;
            const transferAmount = 1; // Minimal amount needed if any
            const transferAmountString = `${transferAmount.toFixed(tokenPrecision)} ${tokenSymbol}`;
            
            // --- Replace mintTokens with direct create/issue calls ---
            const maxSupplyString = `${(1000).toFixed(tokenPrecision)} ${tokenSymbol}`;
            const issueAmountString = `${transferAmount.toFixed(tokenPrecision)} ${tokenSymbol}`;

            // 1. Create the token
            console.log(`[createTestOrg] Attempting token create: ${maxSupplyString} by ${admin.name}`);
            try {
                 await tokenContract.actions.create(
                     [admin.name, maxSupplyString]
                 ).send(`${tokenContract.name}@active`); // Authority of eosio.token contract
                 console.log(`[createTestOrg] Token create SUCCESSFUL`);
            } catch (e) {
                // Ignore if token already created 
                if (!e.message.includes('token symbol already exists')) throw e;
                console.log(`[createTestOrg] Token create skipped (already exists)`);
            }

            // 2. Issue tokens to the admin account
             console.log(`[createTestOrg] Attempting token issue: ${issueAmountString} to ${admin.name}`);
             try {
                 await tokenContract.actions.issue(
                     [admin.name, issueAmountString, 'Initial issue for test']
                 ).send(`${admin.name}@active`); // Authority of the issuer (admin)
                 console.log(`[createTestOrg] Token issue SUCCESSFUL`);
            } catch (e) {
                // Ignore if issue fails potentially due to re-runs or existing tokens
                console.warn(`[createTestOrg] Warning: Issuing tokens failed (might be okay in re-runs): ${e.message}`);
            }
            // -----------------------------------------------------------

            // Minimal subscription setup
             const costExtended = { quantity: transferAmountString, contract: tokenContract.name };
             console.log(`[createTestOrg] Attempting subscription newpack: ${packageName}`);
             try { // Use try-catch in case package already exists
                 await subscriptionContract.actions.newpack([
                     packageName, `Pkg ${orgName}`, 1000n, 3600n, costExtended, true, true
                 ]).send(`${subscriptionContract.name}@active`);
                 console.log(`[createTestOrg] Subscription newpack SUCCESSFUL`);
             } catch (e) {
                 // Ignore if package already exists error
                 if (!e.message.includes('Package already exists')) throw e;
                 console.log(`[createTestOrg] Subscription newpack skipped (already exists)`);
             }

            // Create the org account if it doesn't exist (name might be reused)
            // Redundant if orgName === ORG_ACCOUNT_NAME
            let orgAccount = blockchain.getAccount(orgName);
             console.log(`[createTestOrg] Checking/getting org account: ${orgName}, exists: ${!!orgAccount}`);
            if (!orgAccount && orgName !== ORG_ACCOUNT_NAME) { // Only create if name is different AND doesn't exist
                 console.log(`[createTestOrg] Creating new org account: ${orgName}`);
                 try {
                    orgAccount = await blockchain.createAccount(orgName);
                    console.log(`[createTestOrg] New org account ${orgName} created.`);
                 } catch(e) {
                     if (e.message.includes('Account name already exists')) {
                         orgAccount = blockchain.getAccount(orgName);
                          console.log(`[createTestOrg] Org account ${orgName} already existed.`);
                     } else {
                         throw e;
                     }
                 }
            } else if (!orgAccount && orgName === ORG_ACCOUNT_NAME) {
                 // This case means the initial setup failed or ORG_ACCOUNT_NAME is wrong
                  console.error(`[createTestOrg] CRITICAL: Org account ${orgName} (same as orgContract.name) was expected to exist but not found!`);
                 throw new Error(`Org account ${orgName} was expected to exist but not found.`);
            }
            // Assign package via transfer
            const memo = `${orgName}:${packageName}`;
            console.log(`[createTestOrg] Attempting token transfer for package assignment: ${transferAmountString} from ${admin.name} to ${subscriptionContract.name} with memo: ${memo}`);
            await tokenContract.actions.transfer([
                admin.name, subscriptionContract.name, transferAmountString, memo
            ]).send(`${admin.name}@active`);
            console.log(`[createTestOrg] Token transfer SUCCESSFUL`);

            // Initialize the org
            console.log(`[createTestOrg] Attempting org initorg: org=${orgName}, code=${orgCode}, admin=${admin.name}`);
            try {
                await orgContract.actions.initorg([
                    orgName, orgCode, ipfsImage, orgDisplayName
                ]).send(`${orgName}@active`);
                console.log(`[createTestOrg] Org initorg call SUCCEEDED`);
            } catch (err) {
                console.error(`[createTestOrg] Org initorg call FAILED:`, err);
                throw err; // Re-throw error if initorg explicitly fails
            }

            // Verification that the org exists in the table
            console.log(`[createTestOrg] Verifying org row existence after initorg call...`);
            const orgScope = nameToBigInt(orgContract.name); // Convert scope to BigInt
            const orgsTable = orgContract.tables.orgs(orgScope); // Pass BigInt scope
            const orgPrimaryKeyBigInt = nameToBigInt(ORG_ACCOUNT_NAME); // Convert primary key to BigInt
            const orgRow = await orgsTable.get(orgPrimaryKeyBigInt); // Pass BigInt primary key to .get()
            expect(orgRow, 'Org row should exist after createTestOrg').to.not.be.undefined; // Assert that the row exists

            return orgAccount;
        }

        console.log(">>> About to call createTestOrg"); // Log right before the call
        try {
            // Call the helper function (now defined in this scope)
            await createTestOrg(); // Call the function
            console.log(">>> Finished call to createTestOrg (no error thrown by await)"); // Log after successful await
        } catch (helperError) {
            console.error(">>> ERROR thrown directly by createTestOrg call:", helperError); // Log if await throws
            throw helperError; // Re-throw to ensure the test fails here
        }

        // Verify that the org row was created immediately after createTestOrg
        console.log(">>> Starting verification block"); // Log before verification
        try {
            const orgScope = nameToBigInt(orgContract.name); // Scope is contract account name (as BigInt)
            const orgsTable = orgContract.tables.orgs(orgScope); // Pass BigInt scope
            const orgPrimaryKeyBigInt = nameToBigInt(ORG_ACCOUNT_NAME); // Convert primary key to BigInt
            const orgRow = await orgsTable.get(orgPrimaryKeyBigInt); // Pass BigInt primary key to .get()
            expect(orgRow, 'Org row should exist after createTestOrg').to.not.be.undefined; // Assert that the row exists
        } catch (err) {
            console.error("Error fetching org row in beforeEach:", err); // Log specific error
            // Optional: Dump table state on error for more context
            try {
                const orgScopeDump = nameToBigInt(orgContract.name); // Convert scope to BigInt for dump
                const orgRow = await orgContract.tables.orgs(ORG_ACCOUNT_NAME).get(nameToBigInt(ORG_ACCOUNT_NAME));
                console.error("Current orgs table rows:", orgRow);
            } catch (dumpErr) {
                console.error("Error dumping orgs table:", dumpErr);
            }
            throw err; // Re-throw the original error to fail the test
        }

        // Create subscription package for the org (Only needed for tests that require it later)
        // Might be better to move this into createTestOrg or specific test setups if needed
        // await subscriptionContract.actions.addpackage(['testpkg', true]).send();
    });

    // =========================================================================
    // == ACTION: initorg
    // =========================================================================
    describe('initorg action', () => {
        it('should successfully initialize a new organization', async () => {
            // Arrange: Org and basic setup done in beforeEach
            const orgScope = nameToBigInt(orgContract.name);
            const orgsTable = orgContract.tables.orgs(orgScope);
            const orgPrimaryKeyBigInt = nameToBigInt(ORG_ACCOUNT_NAME);

            // Act: No action needed, setup done in beforeEach

            // Assert: Check if the org exists in the table (already verified in beforeEach, but good practice)
            const orgData = await orgsTable.get(orgPrimaryKeyBigInt);
            expect(orgData).to.not.be.undefined;
            expect(orgData.org_code.toString()).to.equal('test', 'Org code should match');

            // Also verify JSON data if needed
            const onchainData = JSON.parse(orgData.onchain_lookup_data);
            expect(onchainData.user.display_name).to.equal('Test Org for Members');

            const offchainData = JSON.parse(orgData.offchain_lookup_data);
            // Contract stores image as offchainData.user.ipfs_image
            expect(offchainData.user.ipfs_image).to.equal('ipfs://testimage');
        });

        it('should fail if the org account already exists in the orgs table', async () => {
    // Org already created in beforeEach
    // Attempt to initialize the same org account again
    await expect(
        orgContract.actions.initorg({
            org: ORG_ACCOUNT_NAME,
            org_code: 'test',
            ipfs_image: 'ipfs://testimage',
            display_name: 'Test Org'
        }).send(`${ORG_ACCOUNT_NAME}@active`)
    ).to.be.rejectedWith(/org_code must be unique/);

            await expectToThrow(
                orgContract.actions.initorg({
                    org: ORG_ACCOUNT_NAME, 
                    org_code: 'test', // Use valid, lowercase code
                    ipfs_image: 'diff_ipfs', // Different image
                    display_name: 'Diff Name' // Different display name
                }).send(`${ORG_ACCOUNT_NAME}@active`),
                'eosio_assert: org_code must be unique' // Actual contract error
            );
        });

        it('should fail if the org_code is already in use', async () => {
            // Create a *different* org account first for this test
            const secondOrgName = 'secondorgani';
            console.log(`>>> Test: Creating second org account: ${secondOrgName}`);
            const secondOrgAccount = await blockchain.createAccount(secondOrgName);
            console.log(`>>> Test: Second org account created`);

            // Define package name for the second org
            const secondPackageName = `pkg${secondOrgName.substring(0, 8)}`; // Use dynamic name
            console.log(`>>> Test: Second package name: ${secondPackageName}`);

            // Minimal token setup (ensure enough tokens exist on admin)
            const tokenPrecision = 4;
            const transferAmount = 1;
            const transferAmountString = `${transferAmount.toFixed(tokenPrecision)} ${EOS_SYMBOL}`;
            // Ensure admin has enough tokens for transfer
            try {
                const issueResult = await tokenContract.actions.issue([
                    orgAdmin.name, '10.0000 EOS', 'Top up for test'
                ]).send(`${orgAdmin.name}@active`); // Use issuer as sender
                console.log('Token issue result (admin):', JSON.stringify(issueResult));
            } catch (e) {
                console.warn('Token issue error (admin):', e.message);
            }
            // Log admin balance before transfer
            // NOTE: Scope must be nameToBigInt(accountName), symbol code must be a string (e.g. 'EOS')
const adminBalance = await tokenContract.tables.accounts(nameToBigInt(orgAdmin.name)).get(symbolCodeToBigInt(Asset.SymbolCode.from('EOS')));
            console.log('Admin balance before transfer:', JSON.stringify(adminBalance));
            // Top up tokens for secondOrgAccount to avoid overdrawn balance
            try {
                const issueResult = await tokenContract.actions.issue([
                    secondOrgName, '10.0000 EOS', 'Top up for test'
                ]).send(`${orgAdmin.name}@active`); // Use issuer as sender
                console.log('Token issue result (secondOrg):', JSON.stringify(issueResult));
            } catch (e) {
                console.warn('Token issue error (secondOrg):', e.message);
            }
            // Log secondOrg balance before transfer
            const secondOrgBalanceBefore = await tokenContract.tables.accounts(nameToBigInt(secondOrgName)).get(symbolCodeToBigInt(Asset.SymbolCode.from('EOS')));
            console.log('SecondOrg balance before transfer:', JSON.stringify(secondOrgBalanceBefore));

            // Assign the subscription package to the second org (using adminAccount)
            const costExtended = { quantity: transferAmountString, contract: tokenContract.name };
            try {
                await subscriptionContract.actions.assignpkg([
                    secondOrgName, secondPackageName, costExtended
                ]).send(`${adminAccount.name}@active`);
            } catch (e) {
                // Might fail if already assigned
            }
            try { 
                 await subscriptionContract.actions.newpack([
                     secondPackageName, // Use dynamic name
                     `Pkg ${secondOrgName}`, 1000n, 3600n, costExtended, true, true
                 ]).send(`${subscriptionContract.name}@active`);
                 console.log(`>>> Test: Package ${secondPackageName} created`);
             } catch (e) {
                 if (!e.message.includes('Package already exists')) throw e;
                 console.log(`>>> Test: Package ${secondPackageName} already exists`);
             }
            const memo = `${secondOrgName}:${secondPackageName}`; // Use dynamic name
            console.log(`>>> Test: Transferring for package ${secondPackageName} assignment to ${secondOrgName}`);
            await tokenContract.actions.transfer([
                orgAdmin.name, subscriptionContract.name, transferAmountString, memo
            ]).send(`${orgAdmin.name}@active`);
            console.log(`>>> Test: Transfer complete`);


            // Act & Assert: Try to init the second org with the *same* orgCode as the first
            console.log(`>>> Test: Attempting initorg for ${secondOrgName} with duplicate code ${'test'}`);
            // Log transfer string and balance before transfer
            console.log('Transfer string (secondOrg):', transferAmountString);
            // Always use symbol code (e.g. 'EOS') for accounts table queries
            const secondOrgBalanceAfter = await tokenContract.tables.accounts(nameToBigInt(secondOrgName)).get(symbolCodeToBigInt(Asset.SymbolCode.from('EOS')));
            console.log('SecondOrg balance before transfer:', JSON.stringify(secondOrgBalanceBefore));
            const action = orgContract.actions.initorg([
                secondOrgName, // Different org name
                'test',           // SAME org_code
                'ipfs://anotherimage',
                'Second Org'
            ]).send([`${secondOrgName}@active`, `${orgAdmin.name}@active`]); // Authorize

            await expect(action).to.be.rejectedWith(/org_code.*unique/); // Accepts variations
            console.log(`>>> Test: Rejected as expected`);
        });

        it('should fail if org_code is not exactly 4 characters', async () => {
            await expectToThrow(
                orgContract.actions.initorg({
                    org: ORG_ACCOUNT_NAME, // Authorizing account
                    org_code: 'short', // Too short
                    ipfs_image: 'ipfs://testimage',
                    display_name: 'Test Org A'
                }).send(`${ORG_ACCOUNT_NAME}@active`),
                'eosio_assert: org_code must be exactly 4 characters long'
            );
            await expectToThrow(
                orgContract.actions.initorg({
                    org: ORG_ACCOUNT_NAME, // Authorizing account
                    org_code: 'toolong', // Too long
                    ipfs_image: 'ipfs://testimage',
                    display_name: 'Test Org A'
                }).send(`${ORG_ACCOUNT_NAME}@active`),
                'eosio_assert: org_code must be exactly 4 characters long'
            );
        });

        it('should fail if authorization is missing from the org account', async () => {
            // Setup: Create a separate admin account (or use Alice)
            const nonOrgAccount = ALICE_ACCOUNT_NAME; 

            await expectToThrow(
                orgContract.actions.initorg({
                    org: ORG_ACCOUNT_NAME, // Trying to init the contract's own account
                    org_code: 'auth', // Use valid, lowercase code for this test
                    ipfs_image: 'ipfs://auth_fail_image',
                    display_name: 'Auth Fail Org'
                }).send(`${nonOrgAccount}@active`), // Sent by Alice, not the org account
                'missing required authority testorganiza' // Updated expected error
            );
        });

        it('should fail if the org account already exists in the orgs table', async () => {
            // Arrange: The main org (ORG_ACCOUNT_NAME) already exists from beforeEach.
            // We need to ensure it still has a package assigned just in case.
            const mainOrgPackageName = `pkg${ORG_ACCOUNT_NAME.substring(0, 8)}`;
            const transferAmountString = `1.0000 ${EOS_SYMBOL}`;
            const memo = `${ORG_ACCOUNT_NAME}:${mainOrgPackageName}`;
            // Ensure admin has enough tokens for transfer
            try {
                await tokenContract.actions.issue([
                    orgAdmin.name, '10.0000 EOS', 'Top up for test'
                ]).send(`${orgAdmin.name}@active`);
            } catch (e) {
                // Ignore if already issued
            }
            console.log('Transfer string (admin):', transferAmountString);
            // Always use symbol code (e.g. 'EOS') for accounts table queries
            const adminBalanceBefore = await tokenContract.tables.accounts(nameToBigInt(orgAdmin.name)).get(symbolCodeToBigInt(Asset.SymbolCode.from('EOS')));
            console.log('Admin balance before transfer:', JSON.stringify(adminBalanceBefore));
            try {
                console.log(`>>> Test: Re-assigning package ${mainOrgPackageName} to main org ${ORG_ACCOUNT_NAME} (just in case)`);
                await tokenContract.actions.transfer([
                    orgAdmin.name, subscriptionContract.name, transferAmountString, memo
                ]).send(`${orgAdmin.name}@active`);
            } catch (e) {
                // Might fail if token balance insufficient after other tests, log warning
                console.warn(`>>> Test Warning: Re-assigning package failed (might be okay): ${e.message}`);
            }

            // Act & Assert: Try to initialize the *same* org again.
            console.log(`>>> Test: Attempting second initorg for existing org ${ORG_ACCOUNT_NAME}`);
            const action = orgContract.actions.initorg([
                ORG_ACCOUNT_NAME, // SAME org name
                'newc',           // Different org code (must be valid)
                'ipfs://newimage',
                'New Org Display'
            ]).send([`${ORG_ACCOUNT_NAME}@active`, `${orgAdmin.name}@active`]);

            // Expect failure because the org (primary key) already exists in the table.
            await expect(action).to.be.rejectedWith(/Organization.*exists/); // Accepts variations
            console.log(`>>> Test: Rejected as expected (org exists)`);
        });

    });

});
