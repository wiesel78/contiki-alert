import { post } from './rest-client'

export const saveJobsRoute = "/api/v1/job/"

export const saveJobsRest = (clientId, job = {}) =>
    post(saveJobsRoute, {clientId, job});
